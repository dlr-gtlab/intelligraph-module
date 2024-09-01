/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/graphexecmodel.h>
#include <intelli/future.h>

#include <intelli/private/graphexecmodel_impl.h>

#include <intelli/node/groupoutputprovider.h>
#include <intelli/node/groupinputprovider.h>

#include <intelli/connection.h>

#include <gt_eventloop.h>
#include <gt_algorithms.h>

using namespace intelli;

GraphExecutionModel::GraphExecutionModel(Graph& graph) :
    m_graph(&graph)
{
    if (graph.parentGraph())
    {
        gtError() << tr("A graph execution model should only be added to the root Graph!");
        m_modificationCount++; // deactive this exec model
    }

    if (auto* exec = graph.findDirectChild<GraphExecutionModel*>())
    if (exec != this)
    {
        gtError() << tr("The graph %1 already has a graph execution model associated!")
                        .arg(graph.objectName());
    }

    setObjectName(QStringLiteral("__exec_model"));
    setParent(&graph);

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
    connect(this, &GraphExecutionModel::nodeEvaluated,
            this, [this](QString const& nodeUuid){
        auto item = Impl::findData(*this, nodeUuid);
        if (item)
        {
            INTELLI_LOG(*this)
                << tr("node '%1' (%2) evaluated!")
                       .arg(relativeNodePath(*item.node))
                       .arg(item.node->id());
        }
    }, Qt::DirectConnection);

    connect(this, &GraphExecutionModel::nodeEvaluationFailed,
            this, [this](QString const& nodeUuid){
        auto item = Impl::findData(*this, nodeUuid);
        if (item)
        {
            INTELLI_LOG_WARN(*this)
                << tr("node '%1' (%2) failed to evaluate!")
                       .arg(relativeNodePath(*item.node))
                       .arg(item.node->id());
        }
    }, Qt::DirectConnection);

    connect(this, &GraphExecutionModel::internalError,
            this, [this](){
        INTELLI_LOG_WARN(*this) << tr("intenral error occured!");
    }, Qt::DirectConnection);

    connect(this, &GraphExecutionModel::graphStalled,
            this, [this](){
        INTELLI_LOG_WARN(*this) << tr("graph stalled!");
        debug(*this);
    }, Qt::DirectConnection);
#endif

    reset();
}

GraphExecutionModel::~GraphExecutionModel() = default;

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

Graph&
GraphExecutionModel::graph()
{
    assert(m_graph);
    return *m_graph;
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

    if (&this->graph() == &graph)
    {
        connect(&graph, &Graph::globalConnectionAppended,
                this, &GraphExecutionModel::onConnectionAppended,
                Qt::DirectConnection);
        connect(&graph, &Graph::globalConnectionDeleted,
                this, &GraphExecutionModel::onConnectionDeleted,
                Qt::DirectConnection);
    }
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
GraphExecutionModel::beginReset()
{
    m_autoEvaluateRootGraph = false;

    auto iter = m_data.keyBegin();
    auto end  = m_data.keyEnd();
    for (; iter != end; ++iter)
    {
        auto& entry = *m_data.find(*iter);
        entry.isPending = false;
        entry.state = NodeEvalState::Outdated;
        for (auto& entry : entry.portsIn ) entry.data.state = PortDataState::Outdated;
        for (auto& entry : entry.portsOut) entry.data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_targetNodes.clear();
    m_queuedNodes.clear();
    m_data.clear();

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
        << m_modificationCount;

    assert(m_modificationCount >= 0);
    m_modificationCount++;
}

void
GraphExecutionModel::endModification()
{
    m_modificationCount--;
    assert(m_modificationCount >= 0);

    INTELLI_LOG(*this)
        << tr("...END MODIFICATION")
        << m_modificationCount;

    if (m_modificationCount != 0) return;

    Impl::rescheduleTargetNodes(*this);
    Impl::evaluateNextInQueue(*this);
}

bool
GraphExecutionModel::isBeingModified() const
{
    return m_modificationCount > 0;
}

NodeEvalState
GraphExecutionModel::nodeEvalState(NodeUuid const& nodeUuid) const
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item) return NodeEvalState::Invalid;

    // override state
    if (item.isEvaluating()) return NodeEvalState::Evaluating;
    if (!item.node->isActive()) return NodeEvalState::Paused;

    return item->state;
}

bool
GraphExecutionModel::isGraphEvaluated() const
{
    return std::all_of(m_data.keyBegin(), m_data.keyEnd(),
                       [this](NodeUuid const& nodeUuid){
        return isNodeEvaluated(nodeUuid);
    });
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
    return nodeEvalState(nodeUuid) == NodeEvalState::Valid;
}

bool
GraphExecutionModel::isAutoEvaluatingNode(NodeUuid const& nodeUuid) const
{
    auto iter = Impl::findTargetNode(*this, nodeUuid);
    if (iter == m_targetNodes.end()) return false;

    return iter->evalType == NodeEvaluationType::KeepEvaluated;
}

bool
GraphExecutionModel::isAutoEvaluatingGraph() const
{
    return isAutoEvaluatingGraph(this->graph());
}

bool
GraphExecutionModel::isAutoEvaluatingGraph(Graph const& graph) const
{
    if (&this->graph() == &graph)
    {
        return m_autoEvaluateRootGraph;
    }

    return isAutoEvaluatingNode(graph.uuid());
}

ExecFuture
GraphExecutionModel::autoEvaluateGraph()
{
    return autoEvaluateGraph(this->graph());
}

ExecFuture
GraphExecutionModel::autoEvaluateGraph(Graph& graph)
{
    assert(Impl::containsGraph(*this, graph));

    if (&this->graph() == &graph)
    {
        m_autoEvaluateRootGraph = true;
        graph.setActive(true);
    }
    // append graph to target nodes
    else if (!isAutoEvaluatingGraph(graph))
    {
        m_targetNodes.push_back({graph.uuid(), NodeEvaluationType::KeepEvaluated});
    }

    return Impl::evaluateGraph(*this, graph, NodeEvaluationType::KeepEvaluated);
}

ExecFuture
GraphExecutionModel::autoEvaluateNode(NodeUuid const& nodeUuid)
{
    return Impl::evaluateNode(*this, nodeUuid, NodeEvaluationType::KeepEvaluated);
}

ExecFuture
GraphExecutionModel::evaluateGraph()
{
    return evaluateGraph(this->graph());
}

ExecFuture
GraphExecutionModel::evaluateGraph(Graph& graph)
{
    return Impl::evaluateGraph(*this, graph, NodeEvaluationType::SingleShot);
}

ExecFuture
GraphExecutionModel::evaluateNode(NodeUuid const& nodeUuid)
{
    return Impl::evaluateNode(*this, nodeUuid, NodeEvaluationType::SingleShot);
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

    if (&this->graph() == &graph)
    {
        m_autoEvaluateRootGraph = false;
        graph.setActive(false);
    }

    // unschedule all target nodes
    for (auto& conData : graph.localConnectionModel())
    {
        assert(conData.node);
        auto const& nodeUuid = conData.node->uuid();
        Impl::unscheduleNodeRecursively(*this, nodeUuid);
        Impl::removeFromTargetNodes(*this, nodeUuid);
    };

    // we may have unscheduled nodes that are required for other target nodes
    Impl::rescheduleTargetNodes(*this);
}

void
GraphExecutionModel::stopAutoEvaluatingNode(NodeUuid const& nodeUuid)
{
    Impl::unscheduleNodeRecursively(*this, nodeUuid);
    Impl::removeFromTargetNodes(*this, nodeUuid);

    // we may have unscheduled nodes that are required for other target nodes
    Impl::rescheduleTargetNodes(*this);
}

bool
GraphExecutionModel::invalidateNode(NodeUuid const& nodeUuid)
{
    return invalidateNodeOutputs(nodeUuid);
}

bool
GraphExecutionModel::invalidateNodeOutputs(const NodeUuid& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item) return false;

    return Impl::invalidateNodeHelper(*this, nodeUuid, item);
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
    auto item = Impl::findPortData(*this, nodeUuid, portId, setNodeDataError);
    if (!item) return false;

    item->data = std::move(data);

    INTELLI_LOG_SCOPE(*this)
        << tr("setting node data '%4' (%5) for node '%1' (%2, port %3)...")
               .arg(relativeNodePath(*item.node))
               .arg(item.node->id())
               .arg(portId)
               .arg(toString(item->data.ptr), toString(item->data.state));

    switch (item.portType)
    {
    case PortType::In:
    {
        invalidateNodeOutputs(nodeUuid);

        emit item.node->inputDataRecieved(portId);

        if (!item.isEvaluating() && !isBeingModified())
        {
            Impl::rescheduleTargetNodes(*this);
        }
        break;
    }
    case PortType::Out:
    {
        if (item.requiresReevaluation())
        {
            item->data.state = PortDataState::Outdated;
            emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());
        }

        auto& conModel = graph().globalConnectionModel();
        auto* conData = connection_model::find(conModel, nodeUuid);

        assert(conData);

        connection_model::visitSuccessors(*conData, item->portId,
                                          [this, &item](auto& successor){
            setNodeData(successor.node, successor.port, item->data);
        });
        break;
    }
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }

    return true;
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid,
                                 PortType type,
                                 PortIndex portIdx,
                                 NodeDataSet data)
{
    auto item = Impl::findPortData(*this, nodeUuid, type, portIdx, setNodeDataError);
    if (!item) return false;

    return setNodeData(nodeUuid, item.portEntry->portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid,
                                 PortType type,
                                 NodeDataPtrList const& data)
{
    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        INTELLI_LOG_WARN(*this)
            << setNodeDataError(this->graph())
            << tr("node %1 not found!")
                   .arg(nodeUuid);
        return false;
    }

    for (auto& item : data)
    {
        PortId portId = item.first;
        if (!node->port(portId))
        {
            INTELLI_LOG_WARN(*this)
                << setNodeDataError(this->graph())
                << tr("node %1 not found!")
                       .arg(nodeUuid);
            return false;
        }
        if (!setNodeData(nodeUuid, portId, std::move(item.second)))
        {
            return false;
        }
    }

    return true;
}

void
GraphExecutionModel::onNodeEvaluated()
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node)
    {
        gtError()
            << graph().objectName() + QStringLiteral(":")
            << tr("A node has been evaluated, "
                  "but its object was not found!");
        return emit internalError(QPrivateSignal());
    }

    disconnect(node, &Node::computingFinished,
               this, &GraphExecutionModel::onNodeEvaluated);

    INTELLI_LOG_SCOPE(*this)
        << tr("(ASYNC) node '%1' (%2) evaluated!")
               .arg(relativeNodePath(*node))
               .arg(node->id());

    assert(!Impl::isNodeEvaluating(*node));

    NodeUuid const& nodeUuid = node->uuid();

    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError() << graph().objectName() + QStringLiteral(": ")
                  << tr("Node %1 has been evaluated, "
                        "but was not found in the model!")
                         .arg(nodeUuid);
        return emit internalError(QPrivateSignal());
    }

    if (item.requiresReevaluation())
    {
        INTELLI_LOG(*this)
            << tr("node requires reevaluation!");

        emit node->evaluated();

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

    Impl::removeFromTargetNodes(*this, nodeUuid, NodeEvaluationType::SingleShot);

    item->state = NodeEvalState::Valid;
    emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());

    emit nodeEvaluated(nodeUuid, QPrivateSignal());
    emit node->evaluated();

    if (isBeingModified()) return;

    // schedule next nodes marked for evaluation
    auto& conModel = graph().globalConnectionModel();
    auto* conData = connection_model::find(conModel, nodeUuid);
    assert(conData);

    connection_model::visitSuccessors(*conData, [this](auto& successor){
        auto nextNode = Impl::findData(*this, successor.node);
        assert(nextNode);

        if (nextNode->isPending)
        {
            Impl::queueNode(*this, successor.node, nextNode);
        }
    });

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

    if (auto* subgraph = qobject_cast<Graph*>(node))
    {
        setupConnections(*subgraph);

        auto const& nodes = subgraph->nodes();
        for (auto* n : nodes)
        {
            onNodeAppended(n);
        }
    }

    NodeId nodeId = node->id();
    assert(nodeId != invalid<NodeId>());
    NodeUuid const& nodeUuid = node->uuid();
    assert(!nodeUuid.isEmpty());

    if (m_data.contains(nodeUuid))
    {
        INTELLI_LOG(*this)
            << tr("Node %1 already appended!")
                   .arg(nodeUuid);
        return;
    }

    DataItem entry{};
    appendPorts(entry.portsIn, node->ports(PortType::In));
    appendPorts(entry.portsOut, node->ports(PortType::Out));

    INTELLI_LOG(*this)
        << tr("Node %1 (%2:%3) appended!")
               .arg(nodeUuid)
               .arg(nodeId, 2)
               .arg(node->caption());

    m_data.insert(nodeUuid, std::move(entry));

    disconnect(node);

    connect(node, &Node::triggerNodeEvaluation, this, [this, nodeUuid](){
        invalidateNodeOutputs(nodeUuid);
        if (!isBeingModified())
        {
            Impl::rescheduleTargetNodes(*this);
        }
    }, Qt::DirectConnection);

    exec::setNodeDataInterface(*node, *this);

    // update auto evaluating nodes
    auto* graph = Graph::accessGraph(*node);
    assert(graph);
    if (isAutoEvaluatingGraph(*graph))
    {
        Impl::scheduleTargetNode(*this, nodeUuid, NodeEvaluationType::KeepEvaluated);
    }
}

void
GraphExecutionModel::onNodeDeleted(Graph* graph, NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());
    assert(graph);

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Node deleted - cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, *graph, nodeId, makeError);
    if (!item) return;

    INTELLI_LOG(*this)
        << tr("Node deleted - updated execution model! ('%1' (%2))")
               .arg(relativeNodePath(*item.node))
               .arg(nodeId);

    m_data.erase(item.entry);

    Impl::removeFromTargetNodes(*this, item.node->uuid());
    Impl::removeFromQueuedNodes(*this, item.node->uuid());
}

void
GraphExecutionModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert(type != PortType::NoType);
    assert(idx  != invalid<PortIndex>());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
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
        return graph.objectName() + QStringLiteral(": ") +
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

#if 0
void
GraphExecutionModel::onConnectionAppended(Connection* con)
{
    assert(con);
    ConnectionId conId = con->connectionId();
    assert(conId.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection appended: cannot update execution model") + ',';
    };

    auto itemOut = Impl::findData(*this, con->outNodeId(), makeError);
    if (!itemOut) return;

    auto itemIn = Impl::findData(*this, con->inNodeId(), makeError);
    if (!itemIn) return;

    INTELLI_LOG(*this)
        << tr("Connection appended: updated execution model! ('%1')")
               .arg(toString(conId));

    // set node data
    auto data = nodeData(itemOut.node->uuid(), conId.outPort);
    setNodeData(itemIn.node->uuid(), conId.inPort, std::move(data));

    // update auto evaluating nodes
    NodeUuid const& outNodeUuid = itemOut.node->uuid();

    auto* graph = Graph::accessGraph(*itemOut.node);
    if (!isAutoEvaluatingGraph(*graph) ||
        !isAutoEvaluatingNode(outNodeUuid)) return;

    auto& conModel = graph->globalConnectionModel();
    auto* conData  = connection_model::find(conModel, outNodeUuid);
    if (!connection_model::hasPredecessors(conData))
    {
        stopAutoEvaluatingNode(outNodeUuid);
    }
}

void
GraphExecutionModel::onConnectionDeleted(ConnectioId conId)
{
    assert(conId.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection deleted: cannot update execution model") + ',';
    };

    auto itemOut = Impl::findData(*this, conId.outNodeId, makeError);
    if (!itemOut) return;

    auto itemIn = Impl::findData(*this, conId.inNodeId, makeError);
    if (!itemIn) return;

    INTELLI_LOG(*this)
        << tr("Connection deleted: updated execution model! ('%1')")
               .arg(toString(conId));

    // set node data
    NodeDataSet data{nullptr};
    data.state = PortDataState::Valid;

    setNodeData(itemIn.node->uuid(), conId.inPort, data);

    // update auto evaluating nodes
    NodeUuid const& outNodeUuid = itemOut.node->uuid();

    auto* graph = Graph::accessGraph(*itemOut.node);
    if (!isAutoEvaluatingGraph(*graph)) return;

    auto& conModel = graph->globalConnectionModel();
    auto* conData  = connection_model::find(conModel, outNodeUuid);
    if (!connection_model::hasPredecessors(conData))
    {
        Impl::scheduleTargetNode(*this, outNodeUuid, NodeEvaluationType::KeepEvaluated);
    }
}
#endif

void
GraphExecutionModel::onConnectionAppended(ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection appended: cannot update execution model") + ',';
    };

    auto itemOut = Impl::findData(*this, conUuid.outNodeId, makeError);
    if (!itemOut) return;

    auto itemIn = Impl::findData(*this, conUuid.inNodeId, makeError);
    if (!itemIn) return;

    INTELLI_LOG(*this)
        << tr("Connection appended: updated execution model! ('%1')")
               .arg(toString(conUuid));

    // set node data
    auto data = nodeData(itemOut.node->uuid(), conUuid.outPort);
    setNodeData(itemIn.node->uuid(), conUuid.inPort, std::move(data));

    // update auto evaluating nodes
    NodeUuid const& outNodeUuid = itemOut.node->uuid();

    auto* graph = Graph::accessGraph(*itemOut.node);
    if (!isAutoEvaluatingGraph(*graph) ||
        !isAutoEvaluatingNode(outNodeUuid)) return;

    auto& conModel = graph->globalConnectionModel();
    auto* conData  = connection_model::find(conModel, outNodeUuid);
    if (!connection_model::hasPredecessors(conData))
    {
        stopAutoEvaluatingNode(outNodeUuid);
    }
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
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

    setNodeData(itemIn.node->uuid(), conUuid.inPort, data);

    // update auto evaluating nodes
    NodeUuid const& outNodeUuid = itemOut.node->uuid();

    auto* graph = Graph::accessGraph(*itemOut.node);
    if (!isAutoEvaluatingGraph(*graph)) return;

    auto& conModel = graph->globalConnectionModel();
    auto* conData  = connection_model::find(conModel, outNodeUuid);
    if (!connection_model::hasPredecessors(conData))
    {
        Impl::scheduleTargetNode(*this, outNodeUuid, NodeEvaluationType::KeepEvaluated);
    }
}

void
GraphExecutionModel::removeGraphFromModel(Graph* graph)
{
    assert(graph);
    graph->disconnect(this);

    auto const& nodes = graph->nodes();
    for (auto* node : nodes)
    {
//        if (auto* subgraph = qobject_cast<Graph*>(node))
//        {
//            removeGraphFromModel(subgraph);
//        }
        onNodeDeleted(graph, node->id());
    }
}

void
GraphExecutionModel::onGraphDeleted()
{
    Graph* graph = qobject_cast<Graph*>(sender());
    if (!graph)
    {
        gtError()
            << this->graph().objectName() + QStringLiteral(":")
            << tr("A graph node has been delted, "
                  "but its object was not found!");
        return;
    }

    removeGraphFromModel(graph);
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
    QString text;
    auto const& graph = model.graph();
    text = QStringLiteral("Graph: ") + graph.caption() + QStringLiteral("\n");

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
            text += QStringLiteral("\n");
        }
        text += entry.second;
    }

    gtInfo().nospace() << "Debugging graph exec model...\n\"\n" << text << "\"";
}
