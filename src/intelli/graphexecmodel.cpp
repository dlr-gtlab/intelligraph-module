/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/graphexecmodel.h>
#include <intelli/private/graphexecmodel_impl.h>

#include <intelli/node/groupoutputprovider.h>
#include <intelli/node/groupinputprovider.h>

#include <intelli/connection.h>

#include <gt_eventloop.h>

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
                       .arg(item->nodeId);
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
                       .arg(item->nodeId);
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
    entry.state = DataState::RequiresReevaluation;
    for (auto& entry : entry.portsIn ) entry.data.state = PortDataState::Outdated;
    for (auto& entry : entry.portsOut) entry.data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_targetNodes.clear();
    //    m_pendingNodes.clear();
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

    // TODO: explicitly reschedule the graph?
    if (m_modificationCount != 0) return;
}

bool
GraphExecutionModel::isBeingModified() const
{
    return m_modificationCount > 0;
}


NodeEvalState
GraphExecutionModel::nodeEvalState(NodeUuid const& nodeUuid) const
{
    auto find = Impl::findData(*this, nodeUuid);
    if (!find) return NodeEvalState::Invalid;

    // paused
    if (!find.node->isActive())
    {
        return NodeEvalState::Paused;
    }

    // evaluating
    if (find.isEvaluating())
    {
        return NodeEvalState::Evaluating;
    }

    if (find.isEvaluated())
    {
        assert(!find.requiresReevaluation());

        if (find->state == DataState::FailedEvaluation)
        {
            return NodeEvalState::Invalid;
        }
        if (find.node->ports(PortType::Out).empty() && !find.inputsValid())
        {
            return NodeEvalState::Invalid;
        }

        return NodeEvalState::Valid;
    }

    return NodeEvalState::Outdated;
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

void
GraphExecutionModel::setupConnections(Graph& graph)
{
    disconnect(&graph);

    connect(&graph, &Graph::graphAboutToBeDeleted,
            this, [this, &graph](){
        disconnect(&graph);
        auto const& nodes = graph.nodes();
        for (auto* node : nodes) onNodeDeleted(&graph, node->id());
    }, Qt::DirectConnection);

    connect(&graph, &Graph::nodeAppended,
            this, &GraphExecutionModel::onNodeAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::childNodeAboutToBeDeleted,
        this, [this, g = &graph](NodeId nodeId){
            onNodeDeleted(g, nodeId);
        }, Qt::DirectConnection);
    connect(&graph, &Graph::connectionAppended,
            this, &GraphExecutionModel::onConnectionAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::connectionDeleted,
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

FutureEvaluated
GraphExecutionModel::autoEvaluateGraph()
{
    return autoEvaluateGraph(this->graph());
}

FutureEvaluated
GraphExecutionModel::autoEvaluateGraph(Graph& graph)
{
    assert(Impl::isGraphContained(*this, graph));

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

FutureEvaluated
GraphExecutionModel::autoEvaluateNode(NodeUuid const& nodeUuid)
{
    return Impl::evaluateNode(*this, nodeUuid, NodeEvaluationType::KeepEvaluated);
}

FutureEvaluated
GraphExecutionModel::evaluateGraph()
{
    return evaluateGraph(this->graph());
}

FutureEvaluated
GraphExecutionModel::evaluateGraph(Graph& graph)
{
    return Impl::evaluateGraph(*this, graph, NodeEvaluationType::SingleShot);
}

FutureEvaluated
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
    assert(Impl::isGraphContained(*this, graph));

    if (&this->graph() == &graph)
    {
        m_autoEvaluateRootGraph = false;
        graph.setActive(false);
    }

    auto const& nodes = graph.nodes();
    for (auto* node : nodes)
    {
        stopAutoEvaluatingNode(node->uuid());
    }
}

void
GraphExecutionModel::stopAutoEvaluatingNode(NodeUuid const& nodeUuid)
{
    auto iter = Impl::findTargetNode(*this, nodeUuid);
    if (iter != m_targetNodes.end()) m_targetNodes.erase(iter);
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

        switch (item.entry->nodeType)
        {
        // forward to input provider as outputs
        case NodeType::Group:
        {
            auto* graph = qobject_cast<Graph*>(item.node);
            if(!graph) break; // node may no longer be accessible

            auto* input = graph->inputNode();
            if(!input) break;

            auto portIdx = graph->portIndex(PortType::In, portId);
            setNodeData(input->uuid(), input->portId(PortType::Out, portIdx), item->data);
            break;
        }
        // forward to parent graph as outputs
        case NodeType::GroupOutput:
        {
            auto* output = qobject_cast<GroupOutputProvider*>(item.node);
            if(!output) break; // node may no longer be accessible

            auto* graph = qobject_cast<Graph*>(output->parent());
            if(!graph) break;

            auto portIdx = output->portIndex(PortType::In, portId);
            setNodeData(graph->uuid(), graph->portId(PortType::Out, portIdx), item->data);
            break;
        }
        // no forwarding required
        case NodeType::GroupInput:
        case NodeType::Normal:
            break;
        }

        // TODO: trigger next nodes when auto evaluating
        break;
    }
    case PortType::Out:
    {
        if (//item.entry->nodeType != NodeType::GroupInput &&
            (item.requiresReevaluation() ||
             (item.entry->nodeType != NodeType::GroupInput && !item.inputsValid())))
        {
            item->data.state = PortDataState::Outdated;
            emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());
        }

        // forward data to target nodes
        auto const* graph = qobject_cast<Graph const*>(item.node->parent());

        auto const& connections = graph->findConnections(item.node->id(), portId);

        for (ConnectionId con : connections)
        {
            setNodeData(*graph, con.inNodeId, con.inPort, item->data);
        }
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

    return setNodeData(nodeUuid, item.portEntry->id, std::move(data));
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
GraphExecutionModel::onNodeEvaluatedHelper()
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
               this, &GraphExecutionModel::onNodeEvaluatedHelper);

    assert(!Impl::isNodeEvaluating(*node));

    onNodeEvaluated(node->uuid());
}

void
GraphExecutionModel::onNodeEvaluated(QString nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError() << graph().objectName() + QStringLiteral(": ")
                  << tr("Node %1 has been evaluated, "
                        "but was not found in the model!")
                         .arg(nodeUuid);
        return emit internalError(QPrivateSignal());
    }

    INTELLI_LOG_SCOPE(*this)
        << tr("(ASYNC) node '%1' (%2) evaluated!")
               .arg(relativeNodePath(*item.node))
               .arg(item->nodeId);

    emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());

    if (item.requiresReevaluation())
    {
        INTELLI_LOG(*this)
            << tr("node requires reevaluation!");

        // TODO: check if we need to do some more work here.

        emit item.node->evaluated();

        if (!evaluateNode(nodeUuid).detach())
        {
            INTELLI_LOG(*this)
                << tr("failed to reevaluate node '%1' (%2)!")
                       .arg(relativeNodePath(*item.node))
                       .arg(item->nodeId);

            emit nodeEvaluationFailed(nodeUuid, QPrivateSignal());

            emit graphStalled(QPrivateSignal());
        }
        return;
    }

    for (auto& t : m_targetNodes)
    {
        auto item = Impl::findData(*this, t.nodeUuid);
        gtDebug() << "BEFORE" << item.node << t.nodeUuid;
    }

    auto iter = Impl::findTargetNode(*this, nodeUuid);
    if (iter != m_targetNodes.end() &&
        iter->evalType == NodeEvaluationType::SingleShot)
    {
        gtDebug() << "REMOVING SINGLE SHOT" << nodeUuid << item.node << (int)iter->evalType;
        m_targetNodes.erase(iter);
    }

    for (auto& t : m_targetNodes)
    {
        auto item = Impl::findData(*this, t.nodeUuid);
        gtDebug() << "AFTER" << item.node << t.nodeUuid;
    }

    emit nodeEvaluated(nodeUuid, QPrivateSignal());
    emit item.node->evaluated();

    Impl::rescheduleTargetNodes(*this);
    Impl::evaluateNextInQueue(*this);
}

void
GraphExecutionModel::onNodeAppended(Node* node)
{
    auto const appendPorts = [](auto& target, auto const& ports){
        target.reserve(ports.size());

        for (auto& port : ports)
        {
            assert(port.id() != invalid<PortId>());
            target.push_back({port.id()});
        }
    };

    assert(node);

    NodeType type = NodeType::Normal;

    if (auto* g = qobject_cast<Graph*>(node))
    {
        type = NodeType::Group;

        setupConnections(*g);

        auto const& nodes = g->nodes();
        for (auto* n : nodes)
        {
            onNodeAppended(n);
        }
    }
    else if (qobject_cast<GroupInputProvider*>(node))  type = NodeType::GroupInput;
    else if (qobject_cast<GroupOutputProvider*>(node)) type = NodeType::GroupOutput;

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

    DataItem entry{nodeId};
    appendPorts(entry.portsIn, node->ports(PortType::In));
    appendPorts(entry.portsOut, node->ports(PortType::Out));
    entry.nodeType = type;

    INTELLI_LOG(*this)
        << tr("Node %1 (%2:%3) appended!")
               .arg(nodeUuid)
               .arg(nodeId, 2)
               .arg(node->caption());

    m_data.insert(nodeUuid, std::move(entry));

    disconnect(node);

    connect(node, &Node::triggerNodeEvaluation, this, [this, nodeUuid](){
            invalidateNodeOutputs(nodeUuid);
            for (auto& t : m_targetNodes)
            {
                auto item = Impl::findData(*this, t.nodeUuid);
                gtDebug() << "HERE" << item.node << t.nodeUuid;
            }
            Impl::rescheduleTargetNodes(*this);
    }, Qt::DirectConnection);

    exec::setNodeDataInterface(*node, *this);
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
               .arg(item->nodeId);

    m_data.erase(item.entry);

    // remove target node
    auto iter = Impl::findTargetNode(*this, item.node->uuid());
    if (iter != m_targetNodes.end()) m_targetNodes.erase(iter);
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
               .arg(item->nodeId)
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
               .arg(item.portEntry->id);

    item.entry->ports(type).erase(item.portEntry);

    // TODO: reschedule graph
}

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

    auto findOut = Impl::findData(*this, con->outNodeId(), makeError);
    if (!findOut) return;

    auto findIn = Impl::findData(*this, con->inNodeId(), makeError);
    if (!findIn) return;

    // set node data
    auto data = nodeData(findOut.node->uuid(), conId.outPort);
    setNodeData(findIn.node->uuid(), conId.inPort, std::move(data));

    // TODO: reschedule graph
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionId conId)
{
    assert(conId.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection deleted: cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, conId.inNodeId, makeError);
    if (!item) return;

    // set node data
    setNodeData(item.node->uuid(), conId.inPort, nullptr);

    // TODO: reschedule graph
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
                QStringLiteral("STATE: ");

        switch (entry.state)
        {
        case DataState::RequiresReevaluation:
            text += QStringLiteral("RequiresReevaluation\n");
            break;
        case DataState::Evaluated:
            text += QStringLiteral("Evaluated\n");
            break;
        case DataState::FailedEvaluation:
            text += QStringLiteral("FailedEvaluation\n");
            break;
        }

        for (auto* ports : {&entry.portsIn, &entry.portsOut})
        {
            for (auto& port : *ports)
            {
                text +=
                    QString{"  "}.repeated(indent + 1) +
                    QStringLiteral("Port: %1 (%3) - %2 - %4\n")
                        .arg(port.id)
                        .arg(toString(port.data.ptr),
                             toString(node->portType(port.id)),
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

struct FutureEvaluated::Impl
{
    static inline void
    setupEventLoop(FutureEvaluated const& future, GtEventLoop& loop)
    {
        loop.connectFailed(future.m_model.data(), &GraphExecutionModel::internalError);

        // react to node evaluation signals
        auto const& performUpdate = [&future, &loop](){
            future.updateTargets();
            if (future.areNodesEvaluated()) return emit loop.success();
            if (future.haveNodesFailed()) return emit loop.failed();
        };

        QObject::connect(future.m_model, &GraphExecutionModel::nodeEvaluated,
                         &loop, performUpdate);
        QObject::connect(future.m_model, &GraphExecutionModel::nodeEvaluationFailed,
                         &loop, performUpdate);

        performUpdate();
    }

    /// Helper class that invokes the functor member once all target nodes have
    /// evaluated. Exploits GtEventLoop structure.
    struct Observer : public GtEventLoop
    {
        FutureEvaluated future;
        CallbackFunctor functor;

        Observer(FutureEvaluated const& future_,
                 CallbackFunctor functor_) :
            GtEventLoop(0), // event loop wont be executed
            future(future_),
            functor(functor_)
        {
            setObjectName("__observer");
        }
    };
};

FutureEvaluated::FutureEvaluated(GraphExecutionModel& model) :
    m_model(&model)
{
    assert(m_model);
}

FutureEvaluated::FutureEvaluated(GraphExecutionModel& model,
                                         NodeUuid nodeUuid,
                                         NodeEvalState evalState) :
    FutureEvaluated(model)
{
    append(std::move(nodeUuid), evalState);
}

bool
FutureEvaluated::wait(std::chrono::milliseconds timeout) const
{
    GT_INTELLI_PROFILE();

    if (!m_model) return false;

    if (areNodesEvaluated()) return true; // nodes are already evaluated

    if (haveNodesFailed()) return false; // some nodes failed -> abort

    // Nodes are still running
    // -> Create local event loop here to start recieving updates from exec model
    GtEventLoop loop(timeout);
    Impl::setupEventLoop(*this, loop);

    // Nodes may have updated in the meantime (might not be necessary/possible?)
    if (areNodesEvaluated()) return true;
    if (haveNodesFailed()) return false;

    // Perform blocking wait
    GtEventLoop::State state = loop.exec();

    // Reset all targets, so that a subsequent `wait()` has to refetch all
    // their states
    resetTargets();

    return state == GtEventLoop::Success;
}

NodeDataSet
FutureEvaluated::get(NodeUuid const& nodeUuid,
                     PortId portId,
                     milliseconds timeout) const
{
    if (portId == invalid<PortId>()) return {};

    // create a local future to only wait for the one node
    FutureEvaluated future(const_cast<GraphExecutionModel&>(*m_model));
    future.append(nodeUuid, NodeEvalState::Outdated);

    if (!future.wait(timeout)) return {};

    // model may be deleted while waiting
    assert(m_model);
    return m_model->nodeData(nodeUuid, portId);
}

NodeDataSet
FutureEvaluated::get(NodeUuid const& nodeUuid,
                     PortType type,
                     PortIndex portIdx,
                     milliseconds timeout) const
{
    assert(m_model);
    auto* node = m_model->graph().findNodeByUuid(nodeUuid);
    if (!node) return {};

    return get(nodeUuid, node->portId(type, portIdx), timeout);
}

FutureEvaluated const&
FutureEvaluated::then(CallbackFunctor functor) const
{
    auto observer = std::make_unique<Impl::Observer>(*this, std::move(functor));

    auto const invokeFunctor = [o = observer.get()](bool success){
        // invoke functor once, then delete observer
        if (o->functor)
        {
            o->functor(success);
            o->functor = {};
        }
        delete o;
    };
    auto const onSuccess = [invokeFunctor](){
        invokeFunctor(true);
    };
    auto const onFailure = [invokeFunctor](){
        invokeFunctor(false);
    };

    QObject::connect(observer.get(), &Impl::Observer::success,
                     observer.get(), onSuccess);
    QObject::connect(observer.get(), &Impl::Observer::failed,
                     observer.get(), onFailure);
    QObject::connect(observer.get(), &Impl::Observer::abort,
                     observer.get(), onFailure);

    observer->setParent(m_model);

    Impl::setupEventLoop(observer->future, *observer);

    observer.release();

    return *this;
}

bool
FutureEvaluated::detach() const
{
    updateTargets();
    return areNodesEvaluated() || !haveNodesFailed();
}

FutureEvaluated&
FutureEvaluated::join(FutureEvaluated const& other)
{
    if (m_model != other.m_model)
    {
        gtError()
            << QObject::tr("Cannot to join futures, models are incompatible!");
        return *this;
    }

    for (auto const& targets : other.m_targets)
    {
        append(targets.uuid, targets.evalState);
    }
    return *this;
}

FutureEvaluated&
FutureEvaluated::append(NodeUuid nodeUuid, NodeEvalState evalState)
{
    switch (evalState)
    {
    case NodeEvalState::Outdated:
    case NodeEvalState::Evaluating:
        // node is evaluating or was evaluated
        break;
    case NodeEvalState::Paused:
    case NodeEvalState::Invalid:
        evalState = NodeEvalState::Invalid;
        break;
    case NodeEvalState::Valid:
        // only append nodes that are still running or invalid
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
         gtTrace().verbose()
            << "[FutureEvaluated]"
            << QObject::tr("Node %1 finished!").arg(nodeUuid);
#endif
        break;
    }

    auto iter = std::find_if(m_targets.begin(), m_targets.end(),
                             [nodeUuid](TargetNode const& target){
        return target.uuid == nodeUuid;
    });

    // node is already a target, it is unclear which eval state to accept
    // -> mark as outdated
    if (iter != m_targets.end())
    {
        bool isInvalid = iter->evalState == NodeEvalState::Invalid ||
                         evalState != NodeEvalState::Invalid;

        iter->evalState = isInvalid ?
                              NodeEvalState::Invalid :
                              NodeEvalState::Outdated;
        return *this;
    }

    m_targets.push_back({std::move(nodeUuid), evalState});
    return *this;
}

bool
FutureEvaluated::areNodesEvaluated() const
{
    bool allValid = std::all_of(m_targets.begin(), m_targets.end(),
                                [](TargetNode const& target){
        return target.evalState == NodeEvalState::Valid;
    });
    return allValid;
}

bool
FutureEvaluated::haveNodesFailed() const
{
    bool anyInvalid = std::any_of(m_targets.begin(), m_targets.end(),
                                  [](TargetNode const& target){
        return target.evalState == NodeEvalState::Invalid;
    });
    return anyInvalid;
}

void
FutureEvaluated::updateTargets() const
{
    for (auto& target : m_targets)
    {
        NodeEvalState state = m_model->nodeEvalState(target.uuid);
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        if (target.evalState != state)
        {
            gtTrace().verbose()
                << "[FutureEvaluated]"
                << QObject::tr("Node %1 finished!").arg(target.uuid);
        }
#endif
        target.evalState = state;
    }
}

void
FutureEvaluated::resetTargets() const
{
    for (auto& target : m_targets)
    {
        target.evalState = NodeEvalState::Outdated;
    }
}
