/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/graphexecmodel.h>
#include <intelli/graph.h>
#include <intelli/private/graphexecmodel_impl.h>
#include <intelli/node/groupoutputprovider.h>
#include <intelli/node/groupinputprovider.h>

#include <intelli/connection.h>

#include <gt_eventloop.h>
#include <gt_algorithms.h>

using namespace intelli;

GraphExecutionModel::GraphExecutionModel(Graph& graph) :
    pimpl(std::make_unique<Impl>(graph))
{
    if (graph.parentGraph())
    {
        gtError() << utils::logId(this->graph())
                  << utils::logId(*this)
                  << tr("graph %1 is not a root graph!")
                         .arg(graph.objectName());
        pimpl->modificationCount++; // deactivate this exec model
    }

    if (auto* exec = graph.findDirectChild<GraphExecutionModel*>())
    if (exec != this)
    {
        gtWarning() << utils::logId(this->graph())
                    << utils::logId(*this)
                    << tr("graph %1 already has a graph execution model associated!")
                           .arg(graph.objectName());
    }

    setObjectName(QStringLiteral("__exec_model"));
    setParent(&graph);

    // register model in synchronization entity
    {
        QMutexLocker locker{&Impl::s_sync.mutex};
        Impl::s_sync.entries.push_back({this});
    }

    // trigger evalaution of nodes that are potentially waiting for evaluation
    connect(this, &GraphExecutionModel::wakeup, this, [this](){
        if (pimpl->queuedNodes.empty()) return;
        Impl::evaluateNextInQueue(*this);
    }, Qt::QueuedConnection);

#ifndef GT_INTELLI_DEBUG_NODE_EXEC
    connect(this, &GraphExecutionModel::nodeEvaluated,
            this, [this](QString const& nodeUuid){
        Node const* node = this->graph().globalConnectionModel().node(nodeUuid);
        if (node)
        {
            gtInfo().medium()
                << utils::logId(this->graph())
                << utils::logId(*this)
                << tr("node '%1' (%2) evaluated!")
                       .arg(relativeNodePath(*node))
                       .arg(node->id());
        }
    }, Qt::DirectConnection);

    connect(this, &GraphExecutionModel::nodeEvaluationFailed,
            this, [this](QString const& nodeUuid){
        Node const* node = this->graph().globalConnectionModel().node(nodeUuid);
        if (node)
        {
            gtWarning()
                << utils::logId(this->graph())
                << utils::logId(*this)
                << tr("node '%1' (%2) failed to evaluate!")
                       .arg(relativeNodePath(*node))
                       .arg(node->id());
        }
    }, Qt::DirectConnection);
#endif

    connect(this, &GraphExecutionModel::internalError,
            this, [this](){
        gtWarning()
                << utils::logId(this->graph())
                << utils::logId(*this)
                << tr("intenral error occured!");
    }, Qt::DirectConnection);

    connect(this, &GraphExecutionModel::graphStalled,
            this, [this](){
        gtWarning()
                << utils::logId(this->graph())
                << utils::logId(*this)
                << tr("graph stalled!");
    }, Qt::DirectConnection);

    reset();
}

GraphExecutionModel::~GraphExecutionModel()
{
    {
        QMutexLocker locker{&Impl::s_sync.mutex};
        Impl::s_sync.entries.removeAt(Impl::s_sync.indexOf(*this));
    }

    // reset node interface
    gt::for_each_key(pimpl->data, [this](NodeUuid const& nodeUuid){
        Node* node  = graph().findNodeByUuid(nodeUuid);
        if (!node) return;

        exec::setNodeDataInterface(*node, nullptr);
    });
}

GraphExecutionModel*
GraphExecutionModel::accessExecModel(Graph& graph)
{
    auto* root = graph.rootGraph();
    if (!root) return {};

    return root->findDirectChild<GraphExecutionModel*>();
}

GraphExecutionModel const*
GraphExecutionModel::accessExecModel(const Graph& graph)
{
    return accessExecModel(const_cast<Graph&>(graph));
}

GraphExecutionModel*
GraphExecutionModel::make(Graph& graph)
{
    auto* root = &graph;
    assert(root);

    auto* model = accessExecModel(*root);
    if (!model)
    {
        model = new GraphExecutionModel(*root);
    }
    return model;
}

Graph&
GraphExecutionModel::graph()
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

Graph const&
GraphExecutionModel::graph() const
{
    return const_cast<GraphExecutionModel*>(this)->graph();
}

void
GraphExecutionModel::setupConnections(Graph& graph)
{
    graph.disconnect(this);

    connect(&graph, &Graph::graphAboutToBeDeleted,
            this, &GraphExecutionModel::onGraphDeleted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodeAppended,
            this, &GraphExecutionModel::onNodeAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::childNodeAboutToBeDeleted,
            this, [this, g = &graph](NodeId nodeId){
        onNodeDeleted(g, nodeId);
    }, Qt::DirectConnection);

    connect(&graph, &Graph::globalConnectionAppended,
            this, &GraphExecutionModel::onConnectionAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::globalConnectionDeleted,
            this, &GraphExecutionModel::onConnectionDeleted,
            Qt::DirectConnection);

    connect(&graph, &Graph::nodePortInserted,
            this, &GraphExecutionModel::onNodePortInserted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodePortAboutToBeDeleted,
            this, &GraphExecutionModel::onNodePortAboutToBeDeleted,
            Qt::DirectConnection);

    connect(&graph, &Graph::beginModification,
            this, &GraphExecutionModel::onBeginGraphModification,
            Qt::DirectConnection);
    connect(&graph, &Graph::endModification,
            this, &GraphExecutionModel::onEndGraphModification,
            Qt::DirectConnection);
}


void
GraphExecutionModel::reset()
{
    beginModification();

    beginReset();
    endReset();

    endModification();
}

void
GraphExecutionModel::resetTargetNodes()
{
    pimpl->targetNodes.clear();
    pimpl->pendingNodes.clear();
}

void
GraphExecutionModel::beginReset()
{
    assert(pimpl->graph);

    pimpl->autoEvaluatingGraphs.clear();

    auto iter = pimpl->data.keyBegin();
    auto end  = pimpl->data.keyEnd();
    for (; iter != end; ++iter)
    {
        auto& entry = *pimpl->data.find(*iter);
        entry.state = NodeEvalState::Outdated;
        for (auto& e : entry.portsIn ) e.data.state = PortDataState::Outdated;
        for (auto& e : entry.portsOut) e.data.state = PortDataState::Outdated;

        if (Node* node = pimpl->graph->findNodeByUuid(*iter))
        {
            exec::setNodeDataInterface(*node, nullptr);
        }
    }
}

void
GraphExecutionModel::endReset()
{
    pimpl->targetNodes.clear();
    pimpl->queuedNodes.clear();
    pimpl->pendingNodes.clear();
    pimpl->evaluatingNodes.clear();
    pimpl->data.clear();

    Graph& graph = this->graph();
    setupConnections(graph);

    auto const& nodes = graph.nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }
}

void
GraphExecutionModel::beginModification()
{
    INTELLI_LOG(*this)
        << tr("BEGIN MODIFICIATION...")
        << pimpl->modificationCount;

    assert(pimpl->modificationCount >= 0);
    pimpl->modificationCount++;
}

void
GraphExecutionModel::endModification()
{
    pimpl->modificationCount--;
    assert(pimpl->modificationCount >= 0);

    INTELLI_LOG(*this)
        << tr("...END MODIFICATION")
        << pimpl->modificationCount;

    if (pimpl->modificationCount != 0) return;

    Impl::rescheduleTargetNodes(*this);
    Impl::rescheduleAutoEvaluatingNodes(*this);
    Impl::evaluateNextInQueue(*this);
}

bool
GraphExecutionModel::isBeingModified() const
{
    return pimpl->modificationCount > 0;
}

NodeEvalState
GraphExecutionModel::nodeEvalState(NodeUuid const& nodeUuid) const
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item) return NodeEvalState::Invalid;

    if (item.isEvaluating() ||
        item->evaluatingChildNodes > 0)
    {
        return NodeEvalState::Evaluating;
    }
    if (!item.node->isActive())
    {
        return NodeEvalState::Paused;
    }
    return item->state;
}

bool
GraphExecutionModel::isGraphEvaluated() const
{
    return isGraphEvaluated(graph());
}

bool
GraphExecutionModel::isGraphEvaluated(Graph const& graph) const
{
    auto const& nodes = graph.nodes();
    return std::all_of(nodes.begin(), nodes.end(),
                       [this](Node const* node){
       return isNodeEvaluated(node->uuid());
    });
}

bool
GraphExecutionModel::isNodeEvaluated(NodeUuid const& nodeUuid) const
{
    auto iter = pimpl->data.find(nodeUuid);
    return iter != pimpl->data.end() && iter->state == NodeEvalState::Valid;
}

bool
GraphExecutionModel::isEvaluating() const
{
    return !pimpl->evaluatingNodes.empty() || pimpl->isEvaluatingQueue;
}

bool
GraphExecutionModel::isAutoEvaluatingGraph() const
{
    return isAutoEvaluatingGraph(this->graph());
}

bool
GraphExecutionModel::isAutoEvaluatingGraph(Graph const& graph) const
{
    return std::find(pimpl->autoEvaluatingGraphs.begin(),
                     pimpl->autoEvaluatingGraphs.end(),
                     graph.uuid()) != pimpl->autoEvaluatingGraphs.end();
}

bool
GraphExecutionModel::autoEvaluateGraph()
{
    return autoEvaluateGraph(this->graph());
}

bool
GraphExecutionModel::autoEvaluateGraph(Graph& graph)
{
    assert(Impl::containsGraph(*this, graph));

    return Impl::autoEvaluateGraph(*this, graph);
}

ExecFuture
GraphExecutionModel::evaluateGraph()
{
    return evaluateGraph(this->graph());
}

ExecFuture
GraphExecutionModel::evaluateGraph(Graph& graph)
{
    return Impl::evaluateGraph(*this, graph);
}

ExecFuture
GraphExecutionModel::evaluateNode(NodeUuid const& nodeUuid)
{
    return Impl::evaluateNode(*this, nodeUuid);
}

void
GraphExecutionModel::stopAutoEvaluatingGraph()
{
    return stopAutoEvaluatingGraph(this->graph());
}

void
GraphExecutionModel::stopAutoEvaluatingGraph(Graph& graph)
{
    assert(Impl::containsGraph(*this, graph));

    utils::erase(pimpl->autoEvaluatingGraphs, graph.uuid());

    emit autoEvaluationChanged(&graph);

    if (Impl::rescheduleAutoEvaluatingNodes(*this))
    {
        Impl::evaluateNextInQueue(*this);
    }
}

bool
GraphExecutionModel::invalidateNode(NodeUuid const& nodeUuid)
{
    return Impl::invalidateNode(*this, nodeUuid);
}

NodeDataSet
GraphExecutionModel::nodeData(NodeId nodeId, PortId portId) const
{
    return nodeData(graph(), nodeId, portId);
}

NodeDataSet
GraphExecutionModel::nodeData(Graph const& graph,
                              NodeId nodeId,
                              PortId portId) const
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        INTELLI_LOG_WARN(*this)
            << getNodeDataError(graph)
            << tr("node %1 not found!")
                    .arg(nodeId);
        return {};
    }

    return nodeData(node->uuid(), portId);
}

NodeDataSet
GraphExecutionModel::nodeData(NodeUuid const& nodeUuid,
                              PortId portId) const
{
    auto item = Impl::findPortData(*this, nodeUuid, portId, getNodeDataError);
    if (!item) return {};

    return item->data;
}

NodeDataSet
GraphExecutionModel::nodeData(const NodeUuid& nodeUuid,
                              PortType type,
                              PortIndex portIdx) const
{
    auto item = Impl::findPortData(*this, nodeUuid, type, portIdx, getNodeDataError);
    if (!item) return {};

    return item->data;
}

NodeDataPtrList
GraphExecutionModel::nodeData(NodeUuid const& nodeUuid,
                              PortType type) const
{
    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node) return {};

    auto const& ports = node->ports(type);

    NodeDataPtrList list;
    list.reserve(ports.size());
    for (auto const& port : ports)
    {
        list.push_back({port.id(), nodeData(nodeUuid, port.id())});
    }

    return list;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId,
                                 PortId portId,
                                 NodeDataSet data)
{
    return setNodeData(graph(), nodeId, portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(Graph const& graph,
                                 NodeId nodeId,
                                 PortId portId,
                                 NodeDataSet data)
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        INTELLI_LOG_WARN(*this)
            << setNodeDataError(graph)
            << tr("node %1 not found!")
                   .arg(nodeId);
        return false;
    }

    return setNodeData(node->uuid(), portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid,
                                 PortId portId,
                                 NodeDataSet data)
{
    return Impl::setNodeData(*this, nodeUuid, portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid,
                                 PortType type,
                                 PortIndex portIdx,
                                 NodeDataSet data)
{
    auto item = Impl::findPortData(*this, nodeUuid, type, portIdx, setNodeDataError);
    if (!item) return false;

    return Impl::setNodeData(*this, item, std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid,
                                 PortType type,
                                 NodeDataPtrList const& data)
{
    auto item = Impl::findData(*this, nodeUuid, setNodeDataError);
    if (!item) return false;

    for (auto& entry : data)
    {
        PortId portId = entry.first;
        if (!Impl::setNodeData(*this, item, portId, std::move(entry.second)))
        {
            return false;
        }
    }

    return true;
}

GraphDataModel const&
GraphExecutionModel::data() const
{
    return pimpl->data;
}

void
GraphExecutionModel::nodeEvaluationStarted(NodeUuid const& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError()
            << utils::logId(this->graph())
            << utils::logId(*this)
            << tr("Failed to mark node '%1' as evaluating! (node not found)")
                   .arg(nodeUuid);
        return;
    }

    pimpl->evaluatingNodes.push_back(nodeUuid);

    item->state = NodeEvalState::Evaluating;
    emit item.node->nodeEvalStateChanged();

    // update counter for running child nodes
    auto* graph = Graph::accessGraph(*item.node);
    Impl::propagateNodeEvalautionStatus<std::plus>(*this, graph);
}

void
GraphExecutionModel::nodeEvaluationFinished(NodeUuid const& nodeUuid)
{
    utils::erase(pimpl->evaluatingNodes, nodeUuid);

    // update synchronization entity
    Impl::s_sync.update(*this);

    onNodeEvaluated(nodeUuid);
}

void
GraphExecutionModel::setNodeEvaluationFailed(NodeUuid const& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError()
            << utils::logId(this->graph())
            << utils::logId(this)
            << tr("Failed to mark node '%1' as failed! (node not found)")
                   .arg(nodeUuid);
        return;
    }

    Impl::propagateNodeEvaluationFailure(*this, nodeUuid, item);
}

void
GraphExecutionModel::onNodeEvaluated(NodeUuid const& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError()
            << utils::logId(this->graph())
            << utils::logId(*this)
            << tr("Node %1 has been evaluated, "
                  "but was not found in the model!")
                   .arg(nodeUuid);
        return emit internalError(QPrivateSignal());
    }

    Node* node = item.node;

    INTELLI_LOG_SCOPE(*this)
        << tr("node '%1' (%2) evaluated!")
               .arg(relativeNodePath(*node))
               .arg(node->id());

    // update counter for running child nodes
    auto* graph = Graph::accessGraph(*node);
    Impl::propagateNodeEvalautionStatus<std::minus>(*this, graph);

    // TODO: what if the node is required to evaluate a target node?
    if (Impl::isNodeAutoEvaluating(*this, nodeUuid) &&
        item.requiresReevaluation())
    {
        INTELLI_LOG_SCOPE(*this)
            << tr("node requires reevaluation!");

        if (!evaluateNode(nodeUuid).detach())
        {
            INTELLI_LOG(*this)
                << tr("failed to reevaluate node '%1' (%2)!")
                       .arg(relativeNodePath(*node))
                       .arg(node->id());

            emit nodeEvaluationFailed(nodeUuid, QPrivateSignal());

            emit graphStalled(QPrivateSignal());
        }
        return;
    }

    // remove from target nodes
    utils::erase(pimpl->targetNodes, nodeUuid);

    if (item->state != NodeEvalState::Invalid)
    {
        constexpr Impl::SetDataFlags flags = Impl::DontTriggerEvaluation;

        // node not failed -> mark outdated outputs as valid
        for (auto& port : item->portsOut)
        {
            if (port.data.state == PortDataState::Outdated)
            {
                port.data.state = PortDataState::Valid;
                Impl::setNodeData(*this, item,port.portId, port.data, flags);
            }
        }

        item->state = NodeEvalState::Valid;
    }
    emit item.node->nodeEvalStateChanged();

    emit nodeEvaluated(nodeUuid, QPrivateSignal());
    emit node->evaluated();

    if (isBeingModified()) return;

    // trigger successors and next nodes
    if (Impl::isNodeAutoEvaluating(*this, nodeUuid))
    {
        Impl::scheduleAutoEvaluationOfSuccessors(*this, nodeUuid);
    }

    Impl::schedulePendingNodes(*this);

    Impl::evaluateNextInQueue(*this);
}

void
GraphExecutionModel::onNodeAppended(Node* node)
{
    assert(node);

    auto const appendPorts = [](auto& target, auto const& ports){
        target.reserve(ports.size());

        for (auto& port : ports)
        {
            assert(port.id() != invalid<PortId>());
            target.push_back({port.id()});
        }
    };

    NodeUuid const& nodeUuid = node->uuid();
    assert(node->id() != invalid<NodeId>());
    assert(!nodeUuid.isEmpty());

    if (pimpl->data.contains(nodeUuid))
    {
        INTELLI_LOG_WARN(*this)
            << tr("Node %1 already appended!")
                   .arg(nodeUuid);
        return;
    }

    node->disconnect(this);

    // append entry
    DataItem entry{};
    appendPorts(entry.portsIn, node->ports(PortType::In));
    appendPorts(entry.portsOut, node->ports(PortType::Out));

    INTELLI_LOG(*this)
        << tr("Node %1 (%2) appended!")
               .arg(relativeNodePath(*node), nodeUuid);

    pimpl->data.insert(nodeUuid, std::move(entry));

    exec::setNodeDataInterface(*node, this);

    // append subgraph recursively
    if (auto* subgraph = qobject_cast<Graph*>(node))
    {
        // avoid auto evaluating nodes if graph has not been appended fully
        pimpl->modificationCount++;
        auto finally = gt::finally([this](){ pimpl->modificationCount--; });
        Q_UNUSED(finally);

        setupConnections(*subgraph);

        auto const& nodes = subgraph->nodes();
        for (auto* n : nodes)
        {
            onNodeAppended(n);
        }
    }

    // setup connections
    auto autoEvaluate = [this](NodeUuid const& nodeUuid){
        if (isBeingModified()) return;

        if (Impl::isNodeAutoEvaluating(*this, nodeUuid) &&
            Impl::scheduleForAutoEvaluation(*this, nodeUuid))
        {
            Impl::evaluateNextInQueue(*this);
        }
    };

    connect(node, &Node::triggerNodeEvaluation,
            this, [this, nodeUuid, autoEvaluate](){
        invalidateNode(nodeUuid);
        autoEvaluate(nodeUuid);
    }, Qt::DirectConnection);

    connect(node, &Node::isActiveChanged,
            this, [this, nodeUuid, autoEvaluate, node](){
        emit node->nodeEvalStateChanged();
        Node const* n = graph().findNodeByUuid(nodeUuid);
        if (n && n->isActive()) autoEvaluate(nodeUuid);
    }, Qt::DirectConnection);

    // auto evaluate if necessary
    if (!isBeingModified()) Impl::rescheduleAutoEvaluatingNodes(*this);
}

void
GraphExecutionModel::onNodeDeleted(Graph* graph, NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());
    assert(graph);

    auto const makeError = [](Graph const& graph){
        return utils::logId(graph) + QChar{' '} +
               utils::logId<GraphExecutionModel>() + QChar{' '} +
               tr("Node deleted - cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, *graph, nodeId, makeError);
    if (!item) return;

    INTELLI_LOG(*this)
        << tr("Node deleted - updated execution model! ('%1' (%2))")
               .arg(relativeNodePath(*item.node))
               .arg(nodeId);

    pimpl->data.erase(item.entry);

    NodeUuid const& nodeUuid = item.node->uuid();

    utils::erase(pimpl->targetNodes, nodeUuid);
    utils::erase(pimpl->queuedNodes, nodeUuid);
    utils::erase(pimpl->autoEvaluatingGraphs, nodeUuid);
    if (utils::erase(pimpl->evaluatingNodes, nodeUuid))
    {
        // update synchronization entity
        Impl::s_sync.update(*this);
    }
}

void
GraphExecutionModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert(type != PortType::NoType);
    assert(idx  != invalid<PortIndex>());

    auto const makeError = [](Graph const& graph){
        return utils::logId(graph) + QChar{' '} +
               utils::logId<GraphExecutionModel>() + QChar{' '} +
               tr("Port inserted: cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, nodeId, makeError);
    if (!item) return;

    PortId portId = item.node->portId(type, idx);
    assert(portId != invalid<PortId>());

    INTELLI_LOG(*this)
        << tr("Port inserted: updated execution model! ('%1' (%2), port %4)")
               .arg(relativeNodePath(*item.node))
               .arg(item.node->id())
               .arg(portId);

    item.entry->ports(type).push_back({portId});
}

void
GraphExecutionModel::onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert(type != PortType::NoType);
    assert(idx  != invalid<PortIndex>());

    auto const makeError = [](Graph const& graph){
        return utils::logId(graph) + QChar{' '} +
               utils::logId<GraphExecutionModel>() + QChar{' '} +
               tr("Port deleted: cannot update execution model") + ',';
    };

    auto item = Impl::findPortData(*this, nodeId, type, idx, makeError);
    if (!item) return;

    INTELLI_LOG(*this)
        << tr("Port deleted: updated execution model! ('%1' (%2), port %4)")
               .arg(relativeNodePath(*item.node))
               .arg(item.node->id())
               .arg(item.portEntry->portId);

    item.entry->ports(type).erase(item.portEntry);
}

void
GraphExecutionModel::onConnectionAppended(ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto const makeError = [](Graph const& graph){
        return utils::logId(graph) + QChar{' '} +
               utils::logId<GraphExecutionModel>() + QChar{' '} +
               tr("Connection appended: cannot update execution model") + ',';
    };

    auto itemOut = Impl::findData(*this, conUuid.outNodeId, makeError);
    if (!itemOut) return;

    auto itemIn = Impl::findData(*this, conUuid.inNodeId, makeError);
    if (!itemIn) return;

    INTELLI_LOG(*this)
        << tr("Connection appended: updated execution model! ('%1')")
               .arg(toString(conUuid));

    // check if source node is invalid -> propagate invalidation
    bool isInvalid = itemOut->state == NodeEvalState::Invalid;
    if (isInvalid)
    {
        return Impl::propagateNodeEvaluationFailure(*this, conUuid.inNodeId, itemIn);
    }

    // set node data
    auto data = nodeData(itemOut.node->uuid(), conUuid.outPort);
    Impl::setNodeData(*this, itemIn, conUuid.inPort, std::move(data));
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto const makeError = [](Graph const& graph){
        return utils::logId(graph) + QChar{' '} +
               utils::logId<GraphExecutionModel>() + QChar{' '} +
               tr("Connection deleted: cannot update execution model") + ',';
    };

    auto itemOut = Impl::findData(*this, conUuid.outNodeId, makeError);
    if (!itemOut) return;

    auto itemIn = Impl::findData(*this, conUuid.inNodeId, makeError);
    if (!itemIn) return;

    INTELLI_LOG(*this)
        << tr("Connection deleted: updated execution model! ('%1')")
               .arg(toString(conUuid));

    // set node data
    NodeDataSet data{nullptr};
    data.state = PortDataState::Valid;

    Impl::setNodeData(*this, itemIn, conUuid.inPort, std::move(data));
}

void
GraphExecutionModel::onGraphDeleted()
{
    Graph* graph = qobject_cast<Graph*>(sender());
    if (!graph)
    {
        gtError()
            << utils::logId(this->graph())
            << utils::logId(*this)
            << tr("A graph node has been delted, "
                  "but its object was not found!");
        return;
    }

    assert(graph);
    graph->disconnect(this);

    auto const& nodes = graph->nodes();
    for (auto* node : nodes)
    {
        onNodeDeleted(graph, node->id());
    }
}

void
GraphExecutionModel::onBeginGraphModification()
{
    beginModification();
}

void
GraphExecutionModel::onEndGraphModification()
{
    endModification();
}

void
intelli::debug(GraphExecutionModel const& model)
{
    QString debugText;
    auto const& graph = model.graph();
    debugText = QStringLiteral("Graph: ") + graph.caption() + QStringLiteral("\n");

    int base_indent = graph.objectPath().count(';');

    auto const& data = model.data();

    QVector<std::pair<int, QString>> entries;

    auto begin = data.keyValueBegin();
    auto end   = data.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& nodeUuid = iter->first;
        auto& entry = iter->second;

        QString caption QStringLiteral("<NULL_NODE>");
        auto* node = graph.findNodeByUuid(nodeUuid);
        int indent = 1;
        if (node)
        {
            indent = node->objectPath().count(';') - base_indent;
            caption = node->caption();
        }

        entries.push_back({indent, QString{}});
        QString& text = entries.back().second;

        text += QString{"  "}.repeated(indent) +
                QStringLiteral("Node '%1' (%2):\n")
                    .arg(node ? relativeNodePath(*node) : caption, nodeUuid);
        if (!node) continue;

        text += QString{"  "}.repeated(indent + 1) +
                QStringLiteral("STATE: ") +
                toString(entry.state) +
                QStringLiteral("\n");

        for (auto* ports : {&entry.portsIn, &entry.portsOut})
        {
            for (auto& port : *ports)
            {
                text +=
                    QString{"  "}.repeated(indent + 1) +
                    QStringLiteral("Port: %1 (%3) - %2 - %4\n")
                        .arg(port.portId)
                        .arg(toString(port.data.ptr),
                             toString(node->portType(port.portId)),
                             toString(port.data.state));
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b){
        return a.first < b.first;
    });

    int oldIndent = 1;
    for (auto& entry : entries)
    {
        if (oldIndent != entry.first)
        {
            oldIndent = entry.first;
            debugText += QStringLiteral("\n");
        }
        debugText += entry.second;
    }

    gtInfo().nospace() << "Debugging graph exec model...\n\"\n" << debugText << "\"";
}
