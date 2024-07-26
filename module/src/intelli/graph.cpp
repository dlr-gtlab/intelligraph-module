/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/graph.h"

#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/graphexecmodel.h"
#include "intelli/nodeexecutor.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/private/utils.h"
#include "intelli/nodedatafactory.h"

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

#include <QThread>
#include <QCoreApplication>

using namespace intelli;

struct Graph::Impl
{

template<typename MakeError = QString(*)()>
static bool
canAppendConnection(Graph& graph, ConnectionId conId, MakeError makeError = {}, bool silent = true)
{
    if (!conId.isValid())
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(invalid connection)");
        }
        return {};
    }

    // check if nodes differ
    if (conId.inNodeId == conId.outNodeId)
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(connection in-node and out-node are qeual)");
        }
        return {};
    }

    // connection may already exist
    if (graph.findConnection(conId))
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(connection already exists)");
        }
        return {};
    }

    // check if nodes exist
    auto* targetNode = graph.findNodeEntry(conId.inNodeId);
    auto* sourceNode = graph.findNodeEntry(conId.outNodeId);

    if (!targetNode || !sourceNode)
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(connection in-node or out-node was not found)");
        }
        return {};
    }

    assert(targetNode->node->id() == conId.inNodeId &&
           targetNode->node->parent()  == &graph);
    assert(sourceNode->node->id() == conId.outNodeId &&
           sourceNode->node->parent() == &graph);

    // check if ports to connect exist
    auto inPort  = targetNode->node->port(conId.inPort);
    auto outPort = sourceNode->node->port(conId.outPort);

    if (!inPort || !outPort)
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(connection in-port or out-port not found)");
        }
        return {};
    }

    // check if output is connected to input
    if (targetNode->node->portType(inPort->id())  ==
        sourceNode->node->portType(outPort->id()))
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(cannot connect ports of same port type)");
        }
        return {};
    }

    // target node should be an input port
    assert (targetNode->node->portType(inPort->id()) == PortType::In);

    // check if types are compatible
    auto& factory = NodeDataFactory::instance();
    if (!factory.canConvert(outPort->typeId, inPort->typeId))
    {
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(cannot connect ports with incomaptible types)");
        }
        return {};
    }

    // check if input port is already connected
    auto const& cons = graph.findConnections(conId.inNodeId, conId.inPort);
    if (!cons.empty())
    {
        assert (cons.size() == 1);
        if (!silent)
        {
            gtWarning() << makeError()
                        << tr("(in-port is already connected to '%1')").arg(toString(cons.first()));
        }
        return {};
    }

    return true;
}

static bool
accumulateDependentNodes(Graph const& graph, QVector<NodeId>& nodes, NodeId nodeId, PortType type)
{
    auto const* entry = graph.findNodeEntry(nodeId);
    if (!entry) return false;

    for (auto& dependent : type == PortType::In ? entry->ancestors : entry->descendants)
    {
        if (nodes.contains(dependent.node)) continue;
        nodes.append(dependent.node);
        if (!accumulateDependentNodes(graph, nodes, dependent.node, type))
        {
            return false;
        }
    }

    return true;
}

/// checks and updates the node id of the node depending of the policy specified
static bool
updateNodeId(Graph const& graph, Node& node, NodeIdPolicy policy)
{
    auto const nodes = graph.nodes();

    // id may already be used
    QVector<NodeId> ids;
    ids.reserve(nodes.size());
    std::transform(std::begin(nodes), std::end(nodes), std::back_inserter(ids),
                   [](Node const* n){ return n->id(); });

    if (node.id() == invalid<NodeId>() || ids.contains(node.id()))
    {
        if (policy != NodeIdPolicy::Update) return false;

        // generate a new one
        auto maxId = ids.empty() ? 0 : *std::max_element(std::begin(ids), std::end(ids)) + 1;
        node.setId(NodeId::fromValue(maxId));

        return node.id() != invalid<NodeId>();
    }
    return true;
}

/// Functor to handle port deletion
struct PortDeleted
{
    PortDeleted(Graph* g, Node* n) : graph(g), node(n)
    {
        assert(graph);
        assert(node);
    }

    void operator()(PortType type, PortIndex idx)
    {
        NodeId nodeId = node->id();

        auto port = node->portId(type, idx);
        if (port == invalid<PortId>())
        {
            gtWarning() << tr("Failed to update connections of deleted "
                              "port %1 with %2 of node %3!")
                               .arg(port).arg(toString(type)).arg(nodeId);
            return;
        }

        emit graph->nodePortAboutToBeDeleted(nodeId, type, idx);

        auto const& connections = graph->findConnections(nodeId, port);
        if (connections.empty()) return;

        auto cmd = graph->modify();

        for (auto conId : connections)
        {
            graph->deleteConnection(conId);
        }
    }

private:

    Graph* graph = nullptr;
    Node* node = nullptr;
};

/// Functor to handle port change
struct PortChanged
{
    PortChanged(Graph* g, Node* n) : graph(g), node(n)
    {
        assert(graph);
        assert(node);
    }

    void operator()(PortId portId)
    {
        NodeId nodeId = node->id();

        auto const& connections = graph->findConnections(nodeId, portId);
        if (connections.empty()) return;

        PortInfo* port = node->port(portId);
        if (!port)
        {
            gtWarning() << tr("Failed to update connections of changed "
                              "portId %1 node %2!")
                               .arg(portId).arg(nodeId);
            return;
        }

        PortType type = invert(node->portType(portId));
        assert(type != PortType::NoType);

        Modification cmd;
        assert(cmd.isNull());

        auto& factory = NodeDataFactory::instance();

        // check if connections are still valid
        for (auto conId : connections)
        {
            NodeId otherNodeId = conId.node(type);
            assert(otherNodeId != nodeId);

            Node* otherNode = graph->findNode(otherNodeId);
            if (!otherNode) continue;

            PortInfo* otherPort = otherNode->port(conId.port(type));
            if (!otherPort) continue;

            if (!factory.canConvert(port->typeId, otherPort->typeId, invert(type)))
            {
                if (cmd.isNull()) cmd = graph->modify();
                graph->deleteConnection(conId);
            }
        }
    }

private:

    Graph* graph = nullptr;
    Node* node = nullptr;
};

/// Functor to handle node deletion
struct NodeDeleted
{
    NodeDeleted(Graph* g) : graph(g)
    {
        assert(graph);
    }

    void operator()(NodeId nodeId)
    {
        auto node = graph->m_nodes.find(nodeId);
        if (node == graph->m_nodes.end())
        {
            gtWarning() << tr("Failed to delete node") << nodeId
                        << tr("(node was not found!)");
            return;
        }

        auto cmd = graph->modify();

        auto const& connections = graph->findConnections(nodeId);
        for (auto conId : connections)
        {
            graph->deleteConnection(conId);
        }

        graph->m_nodes.erase(node);

        node = graph->m_nodes.find(nodeId);
        if (node != graph->m_nodes.end())
        {
            gtError() << tr("something went wrong?") << nodeId;
        }

        emit graph->nodeDeleted(nodeId);
    }

private:

    Graph* graph = nullptr;
};

/// Functor to handle connection deletion
struct ConnectionDeleted
{
    ConnectionDeleted(Graph* g, ConnectionId id) : graph(g), conId(id)
    {
        assert(graph);
        assert(conId.isValid());
    }

    void operator()()
    {
        auto ancestorConnection   = dag::ConnectionDetail::fromConnection(conId.reversed());
        auto descendantConnection = dag::ConnectionDetail::fromConnection(conId);

        auto* targetNode = graph->findNodeEntry(conId.inNodeId);
        auto* sourceNode = graph->findNodeEntry(conId.outNodeId);

        if (!targetNode || !sourceNode)
        {
            gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                        << tr("(in-node or out-node was not found!)");
            return;
        }

        assert(targetNode->node &&
               targetNode->node->id() == conId.inNodeId &&
               targetNode->node->parent()  == graph);
        assert(sourceNode->node &&
               sourceNode->node->id() == conId.outNodeId &&
               sourceNode->node->parent() == graph);

        auto inIdx  = targetNode->ancestors.indexOf(ancestorConnection);
        auto outIdx = sourceNode->descendants.indexOf(descendantConnection);

        if (inIdx < 0 || outIdx < 0)
        {
            gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                        << tr("(in-connection and out-connection was not found!)")
                        << "in:" << (inIdx >= 0) << "and out:" << (outIdx >= 0);
            return;
        }

        emit targetNode->node->portDisconnected(conId.inPort);
        emit sourceNode->node->portDisconnected(conId.outPort);

        targetNode->ancestors.removeAt(inIdx);
        sourceNode->descendants.removeAt(outIdx);

        emit graph->connectionDeleted(conId);
    }

private:

    Graph* graph = nullptr;
    ConnectionId conId;
};

}; // struct Impl

Graph::Graph() :
    Node("Graph")
{
    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* group = new ConnectionGroup(this);
    group->setDefault(true);

    setActive(false);

    connect(group, &ConnectionGroup::mergeConnections, this, [this](){
        restoreConnections();
    });

    connect(this, &Node::isActiveChanged, this, [this](){
        if (this->findParent<Graph*>()) return;
        if (auto* exec = executionModel())
        {
            isActive() ? (void)exec->autoEvaluate().detach() :
                exec->disableAutoEvaluation();
        }
    });
}

Graph::~Graph() = default;

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

dag::Entry*
Graph::findNodeEntry(NodeId nodeId)
{
    auto iter = m_nodes.find(nodeId);
    if (iter == m_nodes.end()) return {};

    return &(*iter);
}

dag::Entry const*
Graph::findNodeEntry(NodeId nodeId) const
{
    return const_cast<Graph*>(this)->findNodeEntry(nodeId);
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

GraphExecutionModel*
Graph::makeExecutionModel()
{
    auto* model = makeDummyExecutionModel();
    model->makeActive();

    return model;
}

GraphExecutionModel*
Graph::makeDummyExecutionModel()
{
    if (auto* model = executionModel()) return model;

    return new GraphExecutionModel(*this, GraphExecutionModel::DummyModel);
}

GraphExecutionModel*
Graph::executionModel()
{
    return findDirectChild<GraphExecutionModel*>();
}

GraphExecutionModel const*
Graph::executionModel() const
{
    return const_cast<Graph*>(this)->executionModel();
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
    auto* entry = findNodeEntry(nodeId);
    if (!entry) return {};

    assert(entry->node &&
           entry->node->id() == nodeId &&
           entry->node->parent() == this);

    return entry->node;
}

Node const*
Graph::findNode(NodeId nodeId) const
{
    return const_cast<Graph*>(this)->findNode(nodeId);
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
    auto* entry = findNodeEntry(nodeId);
    if (!entry) return {};

    QVector<ConnectionId> connections;

    if (type != PortType::Out) // IN or NoType
    {
        for (auto& con : entry->ancestors)
        {
            connections.append(con.toConnection(nodeId).reversed());
        }
    }
    if (type != PortType::In) // OUT or NoType
    {
        for (auto& con : entry->descendants)
        {
            connections.append(con.toConnection(nodeId));
        }
    }

    return connections;
}

QVector<ConnectionId>
Graph::findConnections(NodeId nodeId, PortId portId) const
{
    auto* entry = findNodeEntry(nodeId);
    if (!entry) return {};

    QVector<ConnectionId> connections;

    for (auto& con : entry->ancestors)
    {
        if (con.sourcePort == portId)
        {
            connections.append(con.toConnection(nodeId).reversed());
        }
        // there should only exist one ingoing connection
        assert(connections.size() <= 1);
    }
    for (auto& con : entry->descendants)
    {
        if (con.sourcePort == portId)
        {
            connections.append(con.toConnection(nodeId));
        }
    }

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
    auto const& connections = findConnections(nodeId, type);
    return uniqueTargetNodes(connections, type);
}

QVector<NodeId>
Graph::findConnectedNodes(NodeId nodeId, PortId portId) const
{
    auto const& connections = findConnections(nodeId, portId);
    auto const* node = findNode(nodeId);
    return uniqueTargetNodes(connections, node ? node->portType(portId) : PortType::Out);
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
        return  tr("Failed to create the connection id!");
    };
    static auto const nodeNotFound = [](NodeId id){
        return tr("(node %1 not found)").arg(id);
    };
    static auto const portOutOfBounds = [](NodeId id, PortIndex idx){
        return tr("(port %1 of node %2 is out of bounds)").arg(idx, id);
    };

    auto* outNode = findNode(outNodeId);
    auto* inNode = findNode(inNodeId);
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

    NodeId nodeId = node->id();

    // check if node is unique
    if (node->nodeFlags() & NodeFlag::Unique)
    {
        auto const& nodes = this->nodes();
        for (Node const* exstNode : nodes)
        {
            if (exstNode->modelName() == node->modelName())
            {
                gtWarning() << makeError()
                            << tr("(node is unique and already exists)");
                return {};
            }
        }
    }

    // append node to hierarchy
    if (!appendChild(node.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    node->updateObjectName();

    // init input output providers of sub graph
    if (auto* graph = qobject_cast<Graph*>(node.get()))
    {
        graph->initInputOutputProviders();
    }

    // append node to model
    m_nodes.insert(nodeId, dag::Entry{ node.get() });

    // setup connections
    connect(node.get(), &Node::portChanged,
            this, Impl::PortChanged(this, node.get()), Qt::DirectConnection);

    connect(node.get(), &Node::portInserted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortInserted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node.get(), &Node::portAboutToBeDeleted,
            this, Impl::PortDeleted(this, node.get()), Qt::DirectConnection);

    connect(node.get(), &Node::portDeleted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortDeleted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node.get(), &Node::nodeAboutToBeDeleted,
            this, Impl::NodeDeleted(this), Qt::DirectConnection);

    // update graph model
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

    if (!Impl::canAppendConnection(*this, conId, makeError, false))
    {
        return {};
    }

    // append connection to hierarchy
    if (!connectionGroup().appendChild(connection.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    connection->updateObjectName();

    // check if nodes exist
    auto* targetNode = findNodeEntry(conId.inNodeId);
    auto* sourceNode = findNodeEntry(conId.outNodeId);
    assert(targetNode);
    assert(sourceNode);

    // append connection to model
    auto ancestorConnection   = dag::ConnectionDetail::fromConnection(conId.reversed());
    auto descendantConnection = dag::ConnectionDetail::fromConnection(conId);

    targetNode->ancestors.append(ancestorConnection);
    sourceNode->descendants.append(descendantConnection);

    // setup connections
    connect(connection.get(), &QObject::destroyed,
            this, Impl::ConnectionDeleted(this, conId), Qt::DirectConnection);

    // update graph model
    emit connectionAppended(connection.get());

    emit targetNode->node->portConnected(conId.inPort);
    emit sourceNode->node->portConnected(conId.outPort);

    return connection.release();
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

    submodel->setNodeData(input->id(), PortType::Out, model.nodeData(id(), PortType::In));
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

void
Graph::onObjectDataMerged()
{
    restoreNodesAndConnections();
}

void
Graph::restoreNode(Node* node)
{
    if (findNode(node->id())) return;

    std::unique_ptr<Node> ptr{node};
    static_cast<QObject*>(node)->setParent(nullptr);

    appendNode(std::move(ptr));
}

void
Graph::restoreConnection(Connection* connection)
{
    if (findConnections(connection->inNodeId(), PortType::In).contains(connection->connectionId()))
    {
        assert(findConnections(connection->outNodeId(), PortType::Out).contains(connection->connectionId()));
        return;
    }
    assert(!findConnections(connection->outNodeId(), PortType::Out).contains(connection->connectionId()));

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

void
intelli::dag::debugGraph(DirectedAcyclicGraph const& graph)
{
    auto const printableCpation = [](Node* node){
        if (!node) return QStringLiteral("NULL");

        auto caption = node->caption();
        caption.remove(']');
        caption.replace('[', '_');
        caption.replace(' ', '_');
        return QStringLiteral("%1:").arg(node->id()) + caption;
    };

    auto text = QStringLiteral("flowchart LR\n");

    auto begin = graph.keyValueBegin();
    auto end   = graph.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& nodeId = iter->first;
        auto& entry = iter->second;

        assert(entry.node && entry.node->id() == nodeId);

        auto caption = printableCpation(entry.node);
        text += QStringLiteral("\t") + caption + QStringLiteral("\n");

        for (auto& data : entry.descendants)
        {
            auto otherEntry = graph.find(data.node);
            if (otherEntry == graph.end()) continue;
            
            assert(otherEntry->node && otherEntry->node->id() == data.node);

            text += QStringLiteral("\t") + caption +
                    QStringLiteral(" --p") + QString::number(data.sourcePort) +
                    QStringLiteral(" : p") + QString::number(data.port) +
                    QStringLiteral("--> ") + printableCpation(otherEntry->node) +
                    QStringLiteral("\n");
        }
    }

    gtInfo().nospace() << "Debugging graph...\n\"\n" << text << "\"";
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

    auto const& connections = graph.findConnections(node.id(), PortType::Out);

    for (auto conId : connections)
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
