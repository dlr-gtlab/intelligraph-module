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

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

using namespace intelli;
using namespace intelli::connection_model;

Graph::Graph() :
    Node("Graph")
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
}

Graph::~Graph() = default;

Graph*
Graph::parentGraph()
{
    return qobject_cast<Graph*>(parent());
}

Graph const*
Graph::parentGraph() const
{
    return qobject_cast<Graph const*>(parent());
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

ConnectionData*
Graph::findNodeEntry(NodeId nodeId)
{
    auto iter = m_data.find(nodeId);
    if (iter == m_data.end()) return {};

    return &(*iter);
}

ConnectionData const*
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
    m_data.insert(nodeId, ConnectionData{ node.get() });

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
    auto ancestorConnection   = ConnectionDetail::fromConnection(conId.reversed());
    auto descendantConnection = ConnectionDetail::fromConnection(conId);

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

void
Graph::eval()
{
    for (auto const& port : ports(PortType::Out))
    {
        setNodeData(port.id(), nodeData(port.id()));
    }
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

namespace intelli
{

inline QString
printableCaption(Node const* node)
{
    if (!node) return QStringLiteral("id{NULL}");

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
    return QStringLiteral("id_%1(%2)").arg(uuid, node->caption());
};

QString
debugGraphHelper(Graph const& graph)
{
    QString text;

    auto const& data = graph.data();

    auto begin = data.keyValueBegin();
    auto end   = data.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& nodeId = iter->first;
        auto& entry = iter->second;

        assert(entry.node && entry.node->id() == nodeId);

        QString const& caption = printableCaption(entry.node);
        text += QStringLiteral("\t") + caption + QStringLiteral("\n");

        if (auto* g = qobject_cast<Graph const*>(entry.node))
        if (auto* input  = g->inputProvider())
        if (auto* output = g->outputProvider())
        {
            text += QStringLiteral("\t") + caption + QStringLiteral("-.->") + printableCaption(input)  + QStringLiteral("\n");
            text += QStringLiteral("\t") + printableCaption(output) + QStringLiteral("-.->") + caption + QStringLiteral("\n");
            text += QStringLiteral("\tsubgraph ") + g->caption() + QStringLiteral("\n");
            text += debugGraphHelper(*g);
            text += QStringLiteral("\tend\n");
        }

        for (auto& desc : entry.descendants)
        {
            auto otherEntry = data.find(desc.node);
            if (otherEntry == data.end()) continue;

            assert(otherEntry->node && otherEntry->node->id() == desc.node);

            text += QStringLiteral("\t") + caption +
                    QStringLiteral(" --") + QString::number(desc.sourcePort) +
                    QStringLiteral(" : ") + QString::number(desc.port) +
                    QStringLiteral("--> ") + printableCaption(otherEntry->node) +
                    QStringLiteral("\n");
        }
    }

    return text;
}

} // namespace intelli

void
intelli::connection_model::debugGraph(ConnectionGraph const&) { }

void
intelli::debug(Graph const& graph)
{
    QString text = QStringLiteral("flowchart LR\n");
    text += debugGraphHelper(graph);

    gtInfo().nospace() << "Debugging graph...\n\"\n" << text << "\"";
}
