/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/graph.h"
#include "intelli/private/graph_impl.h"

#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include "intelli/graphexecmodel.h"
#include "intelli/nodeexecutor.h"

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

using namespace intelli;

Graph::Graph() :
    Node("Graph"),
    m_global(std::make_shared<GlobalConnectionModel>())
{
    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* group = new ConnectionGroup(this);
    group->setDefault(true);

    setActive(false);
    setNodeEvalMode(NodeEvalMode::Blocking);

    connect(group, &ConnectionGroup::mergeConnections, this, [this](){
        restoreConnections();
    });

    connect(this, &Node::isActiveChanged, this, [this](){
        if (this->findParent<Graph*>()) return;
        if (auto* exec = executionModel())
        {
            gtWarning() << __FUNCTION__;
            isActive() ? (void)exec->autoEvaluate().detach() :
                exec->disableAutoEvaluation();
        }
    });
}

Graph::~Graph()
{
    emit graphAboutToBeDeleted(QPrivateSignal());

    auto change = modify();
    Q_UNUSED(change);

    // remove connections
    auto& conGroup = this->connectionGroup();
    delete& conGroup;

    auto const& nodes = this->nodes();
    qDeleteAll(nodes);
}

Graph*
intelli::Graph::accessGraph(Node& node)
{
    return qobject_cast<Graph*>(node.parent());
}

Graph const*
intelli::Graph::accessGraph(Node const& node)
{
    return qobject_cast<Graph const*>(node.parent());
}

Graph*
Graph::parentGraph()
{
    return accessGraph(*this);
}

Graph const*
Graph::parentGraph() const
{
    return accessGraph(*this);
}

Graph*
Graph::rootGraph()
{
    // recursive search
    if (auto p = parentGraph())
    {
        return p->rootGraph();
    }
    return this;
}

Graph const*
Graph::rootGraph() const
{
    return const_cast<Graph*>(this)->rootGraph();
}


QList<Node*>
Graph::nodes()
{
    return findDirectChildren<Node*>();
}

QList<Node const*>
Graph::nodes() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->nodes()
    );
}

QVector<NodeId>
Graph::nodeIds() const
{
    QVector<NodeId> ids;
    auto const& nodes = this->nodes();
    ids.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), ids.begin(),
                   [](Node const* node){ return node->id(); });
    return ids;
}

QList<Connection*>
Graph::connections()
{
    return connectionGroup().findDirectChildren<Connection*>();
}

QList<Connection const*>
Graph::connections() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->connections()
    );
}

QVector<ConnectionId>
Graph::connectionIds() const
{
    QVector<ConnectionId> ids;
    auto const& connections = this->connections();
    ids.resize(connections.size());
    std::transform(connections.begin(), connections.end(), ids.begin(),
                   [](Connection const* con){ return con->connectionId(); });
    return ids;
}

ConnectionGroup&
Graph::connectionGroup()
{
    auto* group = findDirectChild<ConnectionGroup*>();
    assert(group);
    return *group;
}

ConnectionGroup const& Graph::connectionGroup() const
{
    return const_cast<Graph*>(this)->connectionGroup();
}

GroupInputProvider*
Graph::inputProvider()
{
    return findDirectChild<GroupInputProvider*>();
}

GroupInputProvider const*
Graph::inputProvider() const
{
    return const_cast<Graph*>(this)->inputProvider();
}

DynamicNode*
Graph::inputNode()
{
    return inputProvider();
}

DynamicNode const*
Graph::inputNode() const
{
    return inputProvider();
}

GroupOutputProvider*
Graph::outputProvider()
{
    return this->findDirectChild<GroupOutputProvider*>();
}

GroupOutputProvider const*
Graph::outputProvider() const
{
    return const_cast<Graph*>(this)->outputProvider();
}

DynamicNode*
Graph::outputNode()
{
    return outputProvider();
}

DynamicNode const*
Graph::outputNode() const
{
    return outputProvider();
}

QVector<NodeId>
Graph::findDependencies(NodeId nodeId) const
{
    QVector<NodeId> nodes;
    if (!Impl::accumulateDependentNodes(*this, nodes, nodeId, PortType::In))
    {
        return {};
    }
    return nodes;
}

QVector<NodeId>
Graph::findDependentNodes(NodeId nodeId) const
{
    QVector<NodeId> nodes;
    if (!Impl::accumulateDependentNodes(*this, nodes, nodeId, PortType::Out))
    {
        return {};
    }
    return nodes;
}

Node*
Graph::findNode(NodeId nodeId)
{
    auto iter = m_local.find(nodeId);
    if (iter == m_local.end()) return {};

    assert(iter->node &&
           iter->node->id() == nodeId &&
           iter->node->parent() == this);

    return iter->node;
}

Node const*
Graph::findNode(NodeId nodeId) const
{
    return const_cast<Graph*>(this)->findNode(nodeId);
}

Node*
Graph::findNodeByUuid(NodeUuid const& uuid)
{
    return qobject_cast<Node*>(getObjectByUuid(uuid));
}

Node const*
Graph::findNodeByUuid(NodeUuid const& uuid) const
{
    return const_cast<Graph*>(this)->findNodeByUuid(uuid);
}

Connection*
Graph::findConnection(ConnectionId conId)
{
    auto const& childs = connectionGroup().children();
    for (auto* child : childs)
    {
        if (auto* con = qobject_cast<Connection*>(child))
        {
            if (con->connectionId() == conId) return con;
        }
    }
    return nullptr;
}

Connection const*
Graph::findConnection(ConnectionId conId) const
{
    return const_cast<Graph*>(this)->findConnection(conId);
}

QVector<ConnectionId>
Graph::findConnections(NodeId nodeId, PortType type) const
{
    auto iter = m_local.find(nodeId);
    if (iter == m_local.end()) return {};

    QVector<ConnectionId> connections;
    if (type == PortType::NoType)
    {
        std::copy(iter->iterateConnections().begin(),
                  iter->iterateConnections().end(),
                  std::back_inserter(connections));
    }
    else
    {
        std::copy(iter->iterateConnections(type).begin(),
                  iter->iterateConnections(type).end(),
                  std::back_inserter(connections));
    }

    return connections;
}

QVector<ConnectionId>
Graph::findConnections(NodeId nodeId, PortId portId) const
{
    auto iter = m_local.find(nodeId);
    if (iter == m_local.end()) return {};

    QVector<ConnectionId> connections;
    std::copy(iter->iterateConnections(portId).begin(),
              iter->iterateConnections(portId).end(),
              std::back_inserter(connections));
    return connections;
}

QVector<NodeId>
Graph::uniqueTargetNodes(QVector<ConnectionId> const& connections, PortType type)
{
    QVector<NodeId> nodes;
    for (ConnectionId conId : connections)
    {
        if (type == PortType::In) conId = conId.reversed();
        if (!nodes.contains(conId.inNodeId)) nodes.push_back(conId.inNodeId);
    }
    return nodes;
}

QVector<NodeId>
Graph::findConnectedNodes(NodeId nodeId, PortType type) const
{
    QVector<NodeId> nodes;
    auto iter = m_local.iterateUniqueNodes(nodeId, type);
    std::copy(iter.begin(), iter.end(), std::back_inserter(nodes));
    return nodes;
}

QVector<NodeId>
Graph::findConnectedNodes(NodeId nodeId, PortId portId) const
{
    QVector<NodeId> nodes;
    auto iter = m_local.iterateUniqueNodes(nodeId, portId);
    std::copy(iter.begin(), iter.end(), std::back_inserter(nodes));
    return nodes;
}

QList<Graph*>
Graph::graphNodes()
{
    return findDirectChildren<Graph*>();
}

QList<Graph const*>
Graph::graphNodes() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->graphNodes()
    );
}

void
Graph::clearGraph()
{
    // connections should be removed automatically
    qDeleteAll(nodes());
}

PortId
Graph::portId(NodeId nodeId, PortType type, PortIndex portIdx) const
{
    static auto const makeError = [=](){
        return tr("Failed to get port id for node %1!").arg(nodeId);
    };

    auto* node = findNode(nodeId);
    if (!node)
    {
        gtWarning() << makeError() << tr("(node not found)");
        return invalid<PortId>();
    }

    auto port = node->portId(type, portIdx);
    if (port == invalid<PortId>())
    {
        gtWarning() << makeError()
                    << tr("(port idx %1 of type %2 out of bounds)").arg(portIdx).arg(toString(type));
        return invalid<PortId>();
    }

    return port;
}

ConnectionId
Graph::connectionId(NodeId outNodeId, PortIndex outPortIdx, NodeId inNodeId, PortIndex inPortIdx) const
{
    static auto const makeError = [](){
        return  tr("Failed to create connection id!");
    };
    static auto const nodeNotFound = [](NodeId id){
        return tr("(node %1 not found)").arg(id);
    };
    static auto const portOutOfBounds = [](NodeId id, PortIndex idx){
        return tr("(port %1 of node %2 is out of bounds)").arg(idx, id);
    };

    auto* outNode = findNode(outNodeId);
    auto* inNode  = findNode(inNodeId);
    if (!outNode || !inNode)
    {
        gtWarning() << makeError() << (!outNode ? nodeNotFound(outNodeId) :
                                                  nodeNotFound(inNodeId));
        return invalid<ConnectionId>();
    }

    auto outPort = outNode->portId(PortType::Out, outPortIdx);
    auto inPort = inNode->portId(PortType::In, inPortIdx);
    if (outPort == invalid<PortId>() || inPort == invalid<PortId>())
    {
        gtWarning() << makeError() << (outPort == invalid<PortId>() ?
                                           portOutOfBounds(outNodeId, outPortIdx) :
                                           portOutOfBounds(inNodeId, inPortIdx));
        return invalid<ConnectionId>();
    }

    return { outNode->id(), outPort, inNode->id(), inPort };
}

ConnectionUuid
Graph::connectionUuid(ConnectionId conId) const
{
    static auto const makeError = [](){
        return  tr("Failed to create connection uuid!");
    };
    static auto const nodeNotFound = [](NodeId id){
        return tr("(node %1 not found)").arg(id);
    };

    auto* outNode = findNode(conId.outNodeId);
    auto* inNode  = findNode(conId.inNodeId);
    if (!outNode || !inNode)
    {
        gtWarning() << makeError() << (!outNode ? nodeNotFound(conId.outNodeId) :
                                                  nodeNotFound(conId.inNodeId));
        return invalid<ConnectionUuid>();
    }

    return { outNode->uuid(), conId.outPort, inNode->uuid(), conId.inPort };
}

bool
Graph::canAppendConnections(ConnectionId conId)
{
    return Impl::canAppendConnection(*this, conId);
}

Node*
Graph::appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy)
{
    if (!node) return {};

    auto makeError = [n = node.get(), this](){
        return  tr("Failed to append node '%1' to intelli graph '%2'!")
            .arg(n->objectName(), objectName());
    };

    // check if node exists and update node id if necessary
    if (!Impl::updateNodeId(*this, *node, policy))
    {
        gtWarning() << makeError()
                    << tr("(node already exists)");
        return {};
    }

    // check if node can be appended
    if (!Impl::canAppendNode(*this, *node, makeError)) return {};

    // append node to hierarchy
    if (!appendChild(node.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    node->updateObjectName();

    // append nodes of subgraph
    if (auto* graph = qobject_cast<Graph*>(node.get()))
    {
        // merge connection models
        if (this->m_global != graph->m_global)
        {
            this->m_global->insert(*graph->m_global);
        }

        graph->m_global = this->m_global;

        // init input output providers of sub graph
        graph->initInputOutputProviders();
    }

    // register node in local model
    m_local.insert(node->id(), node.get() );

    // register node in global model if not present already (avoid overwrite)
    NodeUuid const& nodeUuid = node->uuid();
    if (!m_global->containts(nodeUuid)) m_global->insert(nodeUuid, node.get());

    // setup connections
    connect(node.get(), &Node::portChanged,
            this, Impl::PortChanged(this, node.get()),
            Qt::DirectConnection);

    connect(node.get(), &Node::portInserted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortInserted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node.get(), &Node::portAboutToBeDeleted,
            this, Impl::PortDeleted(this, node.get()),
            Qt::DirectConnection);

    connect(node.get(), &Node::portDeleted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortDeleted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node.get(), &Node::nodeAboutToBeDeleted,
            this, Impl::NodeDeleted(this),
            Qt::DirectConnection);

    // indicate graph node as evaluating if a node is currently evaluating
    auto updateEvaluatingFlag = [this, node = node.get()](auto const& op){
        assert(node);
//        bool wasActive = m_evaluationIndicator > 0;
        // not incrementing variable since node may emit computing started
        // multiple times
        op(m_evaluationIndicator, node->id());
        bool isActive = m_evaluationIndicator > 0;
        setNodeFlag(Evaluating, isActive);
//        if (wasActive != isActive) emit isActiveChanged();
//        emit isActiveChanged();
    };
    auto setEvaluatingFlag = [updateEvaluatingFlag](){
        updateEvaluatingFlag([](size_t& flags, size_t val){
            flags |= val;
        });
    };
    auto unsetEvluatingFlag = [updateEvaluatingFlag](){
        updateEvaluatingFlag([](size_t& flags, size_t val){
            flags &= ~val;
        });
    };
    connect(node.get(), &Node::computingStarted,
            this, setEvaluatingFlag, Qt::DirectConnection);
    connect(node.get(), &Node::computingFinished,
            this, unsetEvluatingFlag, Qt::DirectConnection);

    // notify
    emit nodeAppended(node.get());

    return node.release();
}

Connection*
Graph::appendConnection(std::unique_ptr<Connection> connection)
{
    if (!connection) return {};

    auto conId = connection->connectionId();

    auto makeError = [conId, this](){
        return tr("Failed to append connection '%1' to intelli graph '%2'!")
            .arg(toString(conId), objectName());
    };

    // check if connection can be appended
    if (!Impl::canAppendConnection(*this, conId, makeError, false)) return {};

    // append connection to hierarchy
    if (!connectionGroup().appendChild(connection.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    connection->updateObjectName();

    // append connection to model
    auto targetNode = m_local.find(conId.inNodeId);
    auto sourceNode = m_local.find(conId.outNodeId);
    assert(targetNode != m_local.end());
    assert(sourceNode != m_local.end());

    auto inConnection  = ConnectionDetail<NodeId>::fromConnection(conId.reversed());
    auto outConnection = ConnectionDetail<NodeId>::fromConnection(conId);

    targetNode->predecessors.append(inConnection);
    sourceNode->successors.append(outConnection);

    // setup connections
    connect(connection.get(), &QObject::destroyed,
            this, Impl::ConnectionDeleted(this, conId),
            Qt::DirectConnection);

    // append to global connection model
    appendGlobalConnection(connection.get(), conId, *targetNode->node);

    // notify
    emit connectionAppended(connection.get());

    auto inPort  = targetNode->node->port(conId.inPort);
    auto outPort = sourceNode->node->port(conId.outPort);
    assert(inPort);
    assert(outPort);

    assert(targetNode->iterateConnections(inPort->id()).size() == 1);
    inPort->setConnected();
    outPort->setConnected();

    emit targetNode->node->portConnected(inPort->id());
    emit sourceNode->node->portConnected(outPort->id());

    return connection.release();
}

void
Graph::appendGlobalConnection(Connection* guard, ConnectionId conId, Node& targetNode)
{
    auto* root = rootGraph();
    assert(root);

    auto conUuid = connectionUuid(conId);
    assert(conUuid.isValid());

    appendGlobalConnection(guard, conUuid);

    // forwards inputs of graph node to subgraph
    if (auto* graph = qobject_cast<Graph*>(&targetNode))
    {
        auto* inputProvider = graph->inputProvider();
        assert(inputProvider);

        conUuid.inNodeId = inputProvider->uuid();
        conUuid.inPort   = GroupInputProvider::virtualPortId(conId.inPort);

        appendGlobalConnection(nullptr, conUuid);
    }

    // forwards outputs of subgraph to graph node
    if (auto* output = qobject_cast<GroupOutputProvider*>(&targetNode))
    {
        NodeUuid const& graphUuid = uuid();

        // graph is being restored (memento diff)
        if (!m_global->containts(graphUuid))
        {
            assert(isBeingModified());
            m_global->insert(graphUuid, this);
        }

        conUuid.outNodeId = output->uuid();
        conUuid.outPort   = GroupOutputProvider::virtualPortId(conUuid.inPort);
        conUuid.inNodeId  = graphUuid;
        conUuid.inPort    = conUuid.outPort;

        appendGlobalConnection(nullptr, std::move(conUuid));
    }
}

void
Graph::appendGlobalConnection(Connection* guard, ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto globalTargetNode = m_global->find(conUuid.inNodeId);
    auto globalSourceNode = m_global->find(conUuid.outNodeId);
    assert(globalTargetNode != m_global->end());
    assert(globalSourceNode != m_global->end());

    auto inConnection  = ConnectionDetail<NodeUuid>::fromConnection(conUuid.reversed());
    auto outConnection = ConnectionDetail<NodeUuid>::fromConnection(conUuid);

    globalTargetNode->predecessors.append(inConnection);
    globalSourceNode->successors.append(outConnection);

    if (guard)
    {
        // setup connections
        connect(guard, &QObject::destroyed,
                this, Impl::GlobalConnectionDeleted(this, conUuid),
                Qt::DirectConnection);
    }

    emit globalConnectionAppended(conUuid);
}

QVector<NodeId>
Graph::appendObjects(std::vector<std::unique_ptr<Node>>& nodes,
                     std::vector<std::unique_ptr<Connection>>& connections)
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    QVector<NodeId> nodeIds;

    for (auto& obj : nodes)
    {
        if (!obj) return nodeIds;

        auto oldId = obj->id();

        auto* node = appendNode(std::move(obj));
        if (!node) return nodeIds;

        auto newId = node->id();

        nodeIds.push_back(newId);

        if (oldId == newId) continue;

        // update connections
        for (auto& con : connections)
        {
            if (con->inNodeId() == oldId)
            {
                con->setInNodeId(newId);
            }
            else if (con->outNodeId() == oldId)
            {
                con->setOutNodeId(newId);
            }
        }
    }

    for (auto& obj : connections)
    {
        // TODO: signal to reciever that the method failed
        if (!appendConnection(std::move(obj))) return nodeIds;
    }

    return nodeIds;
}

bool
Graph::deleteNode(NodeId nodeId)
{
    if (auto* node = findNode(nodeId))
    {
        gtInfo().verbose() << tr("Deleting node:") << node->objectName();
        delete node;
        return true;
    }
    return false;
}

bool
Graph::deleteConnection(ConnectionId connectionId)
{
    if (auto* connection = findConnection(connectionId))
    {
        gtInfo().verbose() << tr("Deleting connection:") << connectionId;
        delete connection;
        return true;
    }
    return false;
}

bool
Graph::isBeingModified() const
{
    return m_modificationCount > 0;
}

void
Graph::onObjectDataMerged()
{
    Node::onObjectDataMerged();

    restoreNodesAndConnections();
}

void
Graph::restoreNode(Node* node)
{
    if (findNode(node->id()))
    {
#ifndef NDEBUG
        if (auto* subgraph = qobject_cast<Graph*>(node))
        {
            assert(subgraph->m_global == m_global);
        }
#endif
        return;
    }

    std::unique_ptr<Node> ptr{node};
    static_cast<QObject*>(node)->setParent(nullptr);

    appendNode(std::move(ptr));
}

void
Graph::restoreConnection(Connection* connection)
{
    assert(connection);
    auto conId = connection->connectionId();

    auto cons = m_local.iterateConnections(conId.inNodeId, PortType::In);
    if (std::find(cons.begin(), cons.end(), conId) != cons.end())
    {
        cons = m_local.iterateConnections(conId.outNodeId, PortType::Out);
        assert(std::find(cons.begin(), cons.end(), conId) != cons.end());
        return;
    }

    cons = m_local.iterateConnections(conId.outNodeId, PortType::Out);
    assert(std::find(cons.begin(), cons.end(), conId) == cons.end());

    std::unique_ptr<Connection> ptr{connection};
    ptr->setParent(nullptr);

    appendConnection(std::move(ptr));
}

void
Graph::restoreConnections()
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    auto const& connections = this->connections();

    for (auto* connection : connections)
    {
        if (!findNode(connection->inNodeId()) ||
            !findNode(connection->outNodeId())) continue;

        restoreConnection(connection);
    }
}

void
Graph::restoreNodesAndConnections()
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    auto const& nodes = this->nodes();
    auto const& connections = this->connections();

    for (auto* node : nodes)
    {
        restoreNode(node);
    }

    for (auto* connection : connections)
    {
        restoreConnection(connection);
    }
}

void
Graph::initInputOutputProviders()
{
    auto* exstInput = inputProvider();
    auto input = exstInput ? nullptr : std::make_unique<GroupInputProvider>();
    
    auto* exstOutput = outputProvider();
    auto output = exstOutput ? nullptr : std::make_unique<GroupOutputProvider>();

    if (input) appendNode(std::move(input));
    if (output) appendNode(std::move(output));
}

void
Graph::eval()
{
//    for (auto& port : ports(PortType::Out))
//    {
//        if (port.visible)
//        {
//            setNodeData(port.id(), nodeData(port.id() + PortId(2)));
//        }
//    }
}

Graph::Modification
Graph::modify()
{
    emitBeginModification();
    return gt::finally(EndModificationFunctor{this});
}

void
Graph::emitBeginModification()
{
    assert(m_modificationCount >= 0);

    m_modificationCount++;
    if (m_modificationCount == 1) emit beginModification(QPrivateSignal());
}

void
Graph::emitEndModification()
{
    m_modificationCount--;

    assert(m_modificationCount >= 0);

    if (m_modificationCount == 0)
    {
        emit endModification(QPrivateSignal());
    }
}

bool
Graph::handleNodeEvaluation(GraphExecutionModel& model)
{
    auto* input = inputProvider();
    if (!input) return false;

    auto* output = outputProvider();
    if (!output) return false;

    auto* submodel = makeDummyExecutionModel();

    gtDebug().verbose().nospace()
        << "### Evaluating node: '" << objectName() << "'";

    emit computingStarted();
    // trick the submodel into thinking that the node was already evaluated
    emit input->computingStarted();

    auto list = model.nodeData(id(), PortType::In);
    list.resize(list.size() - output->ports(PortType::In).size());
    submodel->setNodeData(input->id(), PortType::In, std::move(list));
    submodel->invalidateNode(output->id());

    connect(submodel, &GraphExecutionModel::nodeEvaluated,
            this, &Graph::onSubNodeEvaluated, Qt::UniqueConnection);
    connect(submodel, &GraphExecutionModel::graphStalled,
            this, &Graph::onSubGraphStalled, Qt::UniqueConnection);

    emit input->computingFinished();

    auto finally = gt::finally([this](){
        emit computingFinished();
    });

    if (submodel->evaluateNode(output->id()).detach() &&
        !submodel->isNodeEvaluated(output->id()))
    {
        finally.clear();
    }

    return true;
}

GraphExecutionModel*
Graph::makeExecutionModel()
{
    auto* model = makeDummyExecutionModel();
    model->makeActive();

    return model;
}

GraphExecutionModel*
Graph::executionModel()
{
    return findDirectChild<GraphExecutionModel*>();
}

GraphExecutionModel const*
Graph::executionModel() const
{
    return findDirectChild<GraphExecutionModel*>();
}

GraphExecutionModel*
Graph::makeDummyExecutionModel()
{
    if (auto* model = executionModel()) return model;

    return new GraphExecutionModel(*this, GraphExecutionModel::DummyModel);
}

void
Graph::onSubNodeEvaluated(NodeId nodeId)
{
    auto* output = outputProvider();
    if (!output) return;

    if (output->id() != nodeId) return;

    auto finally = gt::finally([this](){
        emit computingFinished();
    });

    auto* submodel = NodeExecutor::accessExecModel(*output);
    if (!submodel) return;

    auto* model = NodeExecutor::accessExecModel(*this);
    if (!model) return;

    model->setNodeData(id(), PortType::Out, submodel->nodeData(output->id(), PortType::In));
}

void
Graph::onSubGraphStalled()
{
    auto finally = gt::finally([this](){
        emit computingFinished();
    });

    auto* output = outputProvider();
    if (!output) return;

    auto* submodel = NodeExecutor::accessExecModel(*output);
    if (!submodel) return;

    auto* model = NodeExecutor::accessExecModel(*this);
    if (!model) return;

    model->setNodeData(id(), PortType::Out, submodel->nodeData(output->id(), PortType::In));
}

GtMdiItem*
intelli::show(Graph& graph)
{
    return gtMdiLauncher->open(QStringLiteral("intelli::GraphEditor"), &graph);
}

GtMdiItem*
intelli::show(std::unique_ptr<Graph> graph)
{
    if (!graph) return nullptr;

    auto* item = show(*graph);
    if (!item)
    {
        gtWarning() << QObject::tr("Failed to open Graph Editor for intelli graph")
                    << graph->caption();
        return nullptr;
    }

    assert(item->parent() == item->widget());
    assert(item->parent() != nullptr);

    if (item->appendChild(graph.get())) graph.release();

    return item;
}

bool isAcyclicHelper(Graph const& graph,
                     Node const& node,
                     QVector<NodeId>& visited,
                     QVector<NodeId>& pending)
{
    if (pending.contains(node.id()))
    {
        return false;
    }
    if (visited.contains(node.id()))
    {
        return true;
    }

    pending.push_back(node.id());

    auto conData = graph.connectionModel().find(node.id());
    assert(conData != graph.connectionModel().end());

    for (auto conId : conData->iterateConnections(PortType::Out))
    {
        auto* tmp = graph.findNode(conId.inNodeId);
        if (!tmp)
        {
            gtError() << QObject::tr("Failed to check if graph '%1' is acyclic, node %2 not found!")
                             .arg(graph.objectName(), conId.inNodeId);
            return false;
        }
        if (!isAcyclicHelper(graph, *tmp, visited, pending))
        {
            return false;
        }
    }

    visited.push_back(node.id());
    pending.removeOne(node.id());
    return true;
}

QVector<NodeId>
intelli::cyclicNodes(Graph const& graph)
{
    auto const& nodes = graph.nodes();

    QVector<NodeId> visited;
    QVector<NodeId> pending;

    for (auto* node : nodes)
    {
        if (!isAcyclicHelper(graph, *node, visited, pending))
        {
            return pending;
        }
    }

    return pending;
}

namespace intelli
{

inline QString
printableCaption(Node const* node)
{
    if (!node)
    {
        return QStringLiteral("id{NULL}");
    }

    QString uuid = node->uuid();
    uuid.remove('{');
    uuid.remove('}');
    uuid.remove('-');

    if (qobject_cast<GroupInputProvider const*>(node))
    {
        return QStringLiteral("id_%1((I))").arg(uuid);
    }
    if (qobject_cast<GroupOutputProvider const*>(node))
    {
        return QStringLiteral("id_%1((O))").arg(uuid);
    }

    QString caption = node->caption();
    caption.remove(']');
    caption.replace('[', '_');
    caption.replace(' ', '_');
    return QStringLiteral("id_%1(%2)").arg(uuid, caption);
}

QString
debugGraphHelper(Graph const& graph)
{
    QString text;

    auto const& data = graph.globalConnectionModel();

    auto begin = data.keyValueBegin();
    auto end   = data.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& entry = iter->second;

        QString const& caption = printableCaption(entry.node);
        text += QStringLiteral("\t") + caption + QStringLiteral("\n");

        for (auto& con : entry.successors)
        {
            auto otherEntry = data.find(con.node);
            if (otherEntry == data.end()) continue;

            QString entry = QStringLiteral("\t") + caption +
                            QStringLiteral(" --") + QString::number(con.sourcePort) +
                            QStringLiteral(" : ") + QString::number(con.port) +
                            QStringLiteral("--> ") + printableCaption(otherEntry->node) +
                            QStringLiteral("\n");
            if (text.contains(entry)) continue;

            text += entry;
        }

        for (auto& con : entry.predecessors)
        {
            auto otherEntry = data.find(con.node);
            if (otherEntry == data.end()) continue;

            QString entry = QStringLiteral("\t") + printableCaption(otherEntry->node) +
                            QStringLiteral(" --") + QString::number(con.port) +
                            QStringLiteral(" : ") + QString::number(con.sourcePort) +
                            QStringLiteral("--> ") + caption +
                            QStringLiteral("\n");
            if (text.contains(entry)) continue;

            text += entry;
        }
    }

    return text;
}

} // namespace intelli

void
intelli::debug(Graph const& graph)
{
    QString text = QStringLiteral("flowchart LR\n");
    text += debugGraphHelper(graph);

    gtInfo().nospace() << "Debugging graph...\n\"\n" << text << "\"";
}
