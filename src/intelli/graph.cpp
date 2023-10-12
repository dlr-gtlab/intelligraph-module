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
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

#include <QThread>
#include <QCoreApplication>

using namespace intelli;

/// checks and updates the node id of the node depending of the policy specified
bool
updateNodeId(Graph const& graph, Node& node, NodeIdPolicy policy)
{
    auto const nodes = graph.nodes();

    // id may already be used
    QVector<NodeId> ids;
    ids.reserve(nodes.size());
    std::transform(std::begin(nodes), std::end(nodes), std::back_inserter(ids),
                   [](Node const* n){ return n->id(); });

    if (ids.contains(node.id()))
    {
        if (policy != UpdateNodeId) return false;

        // generate a new one
        auto maxId = *std::max_element(std::begin(ids), std::end(ids)) + 1;
        node.setId(NodeId::fromValue(maxId));
        assert(node.id() != invalid<NodeId>());
        return true;
    }
    return true;
}

Graph::Graph() :
    Node("Graph")
{
    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* group = new ConnectionGroup(this);
    group->setDefault(true);

    connect(group, &ConnectionGroup::mergeConnections, this, [this](){
        auto const& connections = this->connections();

        for (auto* connection : connections)
        {
            if (!findNode(connection->inNodeId()) ||
                !findNode(connection->outNodeId())) continue;

            restoreConnection(connection);
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
Graph::findConnections(NodeId nodeId, PortType type, PortIndex idx) const
{
    auto* entry = findNodeEntry(nodeId);
    if (!entry) return {};

    QVector<ConnectionId> connections;

    if (type == PortType::In)
    {
        for (auto& con : entry->ancestors)
        {
            if (con.sourcePort == idx)
            {
                connections.append(con.toConnection(nodeId).reversed());
            }
        }
    }
    else if (type == PortType::Out) // OUT
    {
        for (auto& con : entry->descendants)
        {
            if (con.sourcePort == idx)
            {
                connections.append(con.toConnection(nodeId));
            }
        }
    }

    return connections;
}

QVector<NodeId>
Graph::findTargetNodes(NodeId nodeId, PortType type, PortIndex idx) const
{
    auto const& connections = idx == invalid<PortIndex>() ?
        findConnections(nodeId, type) :
        findConnections(nodeId, type, idx);

    QVector<NodeId> nodes;
    for (ConnectionId conId : connections)
    {
        if (!nodes.contains(conId.inNodeId)) nodes.push_back(conId.inNodeId);
    }
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
Graph::clear()
{
    qDeleteAll(nodes());
}

Node*
Graph::appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy)
{
    auto makeError = [n = node.get()](){
        return  tr("Failed to append node '%1' to intelli graph!")
                .arg(n->objectName());
    };

    if (!node) return {};

    if (!updateNodeId(*this, *node, policy))
    {
        gtWarning() << makeError()
                    << tr("(node id '%2' already exists)").arg(node->id());
        return {};
    }

    if (!appendChild(node.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    // init input output providers of sub graph
    if (auto* graph = qobject_cast<Graph*>(node.get()))
    {
        graph->initInputOutputProviders();
    }

    node->updateObjectName();

    NodeId nodeId = node->id();

    gtInfo().medium() << tr("Appending node to map: %1 (id: %2)")
                             .arg(node->objectName()).arg(nodeId);

    m_nodes.insert(nodeId, dag::Entry{ node.get() });

    connect(node.get(), &QObject::destroyed, this, [this, nodeId](){
        gtInfo() << tr("Deleting node") << nodeId << tr("from map");

        auto node = m_nodes.find(nodeId);
        if (node == m_nodes.end())
        {
            gtWarning() << tr("Failed to delete node") << nodeId
                        << tr("(node was not found!)");
            return;
        }

        for (auto nodeData : node->ancestors)
        {
            deleteConnection(nodeData.toConnection(nodeId).reversed());
        }

        for (auto nodeData : node->descendants)
        {
            deleteConnection(nodeData.toConnection(nodeId));
        }

        m_nodes.erase(node);

        emit nodeDeleted(nodeId);
    });

    // update graph model
    emit nodeAppended(node.get());

    return node.release();
}

Connection*
Graph::appendConnection(std::unique_ptr<Connection> connection)
{
    auto makeError = [c = connection.get()](){
        return tr("Failed to append connection '%1' to intelli graph!")
               .arg(c->objectName());
    };

    // connection may already exist
    if (!connection) return {};

    connection->updateObjectName();

    auto conId = connection->connectionId();

    if (findConnection(conId))
    {
        gtWarning() << makeError()
                    << tr("(connection already exists)");
        return {};
    }

    if (!connectionGroup().appendChild(connection.get()))
    {
        gtWarning() << makeError();
        return {};
    }

    auto* targetNode = findNodeEntry(conId.inNodeId);
    auto* sourceNode = findNodeEntry(conId.outNodeId);

    if (!targetNode || !sourceNode)
    {
        gtWarning() << makeError()
                    << tr("(connection in-node or out-node was not found!)");
        return {};
    }

    assert(targetNode->node &&
           targetNode->node->id() == conId.inNodeId &&
           targetNode->node->parent()  == this);
    assert(sourceNode->node &&
           sourceNode->node->id() == conId.outNodeId &&
           sourceNode->node->parent() == this);

    auto inPorts  = targetNode->node->ports(PortType::In).size();
    auto outPorts = sourceNode->node->ports(PortType::Out).size();

    if (inPorts <= conId.inPortIndex || outPorts <= conId.outPortIndex)
    {
        gtWarning() << makeError()
                    << tr("(connection in-port or out-port is out of bounds!)")
                    << inPorts << "vs" << conId.inPortIndex << "and"
                    << outPorts << "vs" << conId.outPortIndex;
        return {};
    }

    gtInfo() << tr("Appending connection") << conId << tr("to map");

    auto ancestorConnection   = dag::ConnectionDetail::fromConnection(conId.reversed());
    auto descendantConnection = dag::ConnectionDetail::fromConnection(conId);

    targetNode->ancestors.append(ancestorConnection);
    sourceNode->descendants.append(descendantConnection);

    connect(connection.get(), &QObject::destroyed,
            this, [this, conId, ancestorConnection, descendantConnection](){
        auto* targetNode = findNodeEntry(conId.inNodeId);
        auto* sourceNode = findNodeEntry(conId.outNodeId);

        if (!targetNode || !sourceNode)
        {
            gtWarning() << tr("Failed to delete connection") << conId
                        << tr("(in-node or out-node was not found!)");
            return;
        }

        assert(!targetNode->node ||
               (targetNode->node->id()  == conId.inNodeId &&
                targetNode->node->parent()  == this));
        assert(!sourceNode->node ||
               (sourceNode->node->id() == conId.outNodeId &&
                sourceNode->node->parent() == this));

        auto inIdx  = targetNode->ancestors.indexOf(ancestorConnection);
        auto outIdx = sourceNode->descendants.indexOf(descendantConnection);

        if (inIdx < 0 || outIdx < 0)
        {
            gtWarning() << tr("Failed to delete connection") << conId
                        << tr("(in-connection and out-connection was not found!)")
                        << "in:" << (inIdx >= 0) << "and out:" << (outIdx >= 0);
            return;
        }

        gtInfo() << tr("Deleting connection") << conId << tr("from map");

        targetNode->ancestors.remove(inIdx);
        sourceNode->descendants.remove(outIdx);

        emit connectionDeleted(conId);
    });

    // update graph model
    emit connectionAppended(connection.get());

    return connection.release();
}

QVector<NodeId>
Graph::appendObjects(std::vector<std::unique_ptr<Node>>& nodes,
                     std::vector<std::unique_ptr<Connection>>& connections)
{
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
        gtInfo().verbose() << tr("Updating node id from %1 to %2...").arg(oldId).arg(newId);

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
        auto* con = appendConnection(std::move(obj));
        if (!con) return nodeIds;
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
Graph::handleNodeEvaluation(GraphExecutionModel& model, PortIndex outIdx)
{
    auto* input = inputProvider();
    if (!input) return false;

    auto* submodel = findDirectChild<GraphExecutionModel*>();
    if (submodel == &model || model.parent() == this)
    {
        gtError() << tr("Unexpected graph execution model!");
        return false;
    }

    if (!submodel) submodel = new GraphExecutionModel(*this);
    submodel->setParent(this);

    auto size = ports(PortType::In).size();

    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << objectName()
        << "' at output idx '" << outIdx << "'";

    for (PortIndex idx(0); idx < size; ++idx)
    {
        auto* port = model.findPortDataEntry(id(), PortType::In, idx);
        assert(port);
        submodel->setNodeData(input->id(), PortType::Out, idx, port->data);
    }

    connect(submodel, &GraphExecutionModel::nodeEvaluated,
            this, &Graph::onOutputProivderEvaluated, Qt::UniqueConnection);

    emit computingStarted();

    auto finally = gt::finally([this](){
        emit computingFinished();
    });

    if (!submodel->autoEvaluate()) return false;

    finally.clear();

    return true;
}

void
Graph::onObjectDataMerged()
{
    gtDebug().verbose() << __FUNCTION__;

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
Graph::onOutputProivderEvaluated(NodeId nodeId)
{
    auto* output = outputProvider();
    if (!output) return;

    if (output->id() != nodeId) return;

    auto finally = gt::finally([this](){
        emit computingFinished();
    });

    auto* submodel = findDirectChild<GraphExecutionModel*>();
    if (!submodel) return;

    auto* model = executionModel();
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
            gtError() << QObject::tr("Failed to check if graph is acyclic, node %1 not found!")
                             .arg(conId.inNodeId);
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
intelli::cyclicNodes(Graph& graph)
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
