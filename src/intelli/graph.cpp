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
#include "intelli/nodefactory.h"
#include "intelli/adapter/modeladapter.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/gui/grapheditor.h"

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

#include <QThread>
#include <QCoreApplication>

using namespace intelli;

auto init_once = [](){
    return GT_INTELLI_REGISTER_NODE(Graph, "Group")
}();

template <typename ObjectList, typename T = gt::trait::value_t<ObjectList>>
inline T findNode(ObjectList const& nodes,
                  QtNodes::NodeId nodeId)
{
    auto iter = std::find_if(std::begin(nodes), std::end(nodes),
                             [=](Node const* node) {
        return node->id() == nodeId;
    });

    return iter == std::end(nodes) ? nullptr : *iter;
}

template <typename ObjectList, typename T = gt::trait::value_t<ObjectList>>
inline T findConnection(ObjectList const& connections,
                        intelli::ConnectionId conId)
{
    auto iter = std::find_if(std::begin(connections), std::end(connections),
                             [&](Connection* connection){
        return conId == connection->connectionId();
    });

    return iter == std::end(connections) ? nullptr : *iter;
}

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
        if (auto* adapter = findModelAdapter())
        {
            adapter->mergeConnections(*this);
        }
    });
}

Graph::~Graph()
{
    // we should stop the execution of the model manager first
    // (if its still active)
    delete findModelAdapter();
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
    return ::findNode(nodes(), nodeId);
}

Node const*
Graph::findNode(NodeId nodeId) const
{
    return const_cast<Graph*>(this)->findNode(nodeId);
}

Connection*
Graph::findConnection(ConnectionId const& conId)
{
    return ::findConnection(connections(), conId);
}

Connection const*
Graph::findConnection(ConnectionId const& conId) const
{
    return const_cast<Graph*>(this)->findConnection(conId);
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

ModelAdapter*
Graph::findModelAdapter()
{
    return findDirectChild<ModelAdapter*>();
}

ModelAdapter const*
Graph::findModelAdapter() const
{
    return const_cast<Graph*>(this)->findModelAdapter();
}

void
Graph::clear()
{
    qDeleteAll(connections());
    qDeleteAll(nodes());
}

Node*
Graph::appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy)
{
    if (!node) return {};

    if (!updateNodeId(*this, *node, policy))
    {
        gtWarning() << tr("Failed to append node '%1' to intelli graph! "
                          "(node id '%2' already exists)")
                           .arg(node->objectName()).arg(node->id());
        return {};
    }

    gtInfo().medium() << tr("Appending node: %1 (id: %2) ")
                             .arg(node->objectName()).arg(node->id());

    if (!appendChild(node.get()))
    {
        gtWarning() << tr("Failed to append node '%1' to intelli graph!")
                           .arg(node->objectName());
        return {};
    }

    // init input output providers of sub graph
    if (auto* graph = qobject_cast<Graph*>(node.get()))
    {
        graph->initInputOutputProviders();
    }

    node->updateObjectName();

    // update graph model
    emit nodeAppended(node.get());

    return node.release();
}

Connection*
Graph::appendConnection(std::unique_ptr<Connection> connection)
{
    // connection may already exist
    if (!connection) return {};

    connection->updateObjectName();

    if (findConnection(connection->connectionId()))
    {
        gtWarning() << tr("Failed to append connection '%1' to intelli graph! "
                          "(connection already exists)")
                           .arg(connection->objectName());
        return {};
    }

    gtInfo().medium() << tr("Appending connection:") << connection->connectionId();

    if (!connectionGroup().appendChild(connection.get()))
    {
        gtWarning() << tr("Failed to append connection '%1' to intelli graph!")
                           .arg(connection->objectName());
        return {};
    }

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

void
Graph::setNodePosition(Node* node, QPointF pos)
{
    if (node)
    {
        node->setPos(pos);
        emit nodePositionChanged(node->id(), pos);
    }
}

ModelAdapter*
Graph::makeModelAdapter(ModelPolicy policy)
{
    if (auto* adapter = findModelAdapter())
    {
        // upgrade dummy model to active model
        if (policy == ActiveModel)
        {
            adapter->setModelPolicy(policy);
        }
        return adapter;
    }

    return new ModelAdapter(*this, policy);
}

void
Graph::clearModelAdapter(bool force)
{
    auto* adapter = findModelAdapter();
    if (!adapter)
    {
        gtWarning() << tr("Model adapter not found!");
        return;
    }

    if (!adapter->readyForRemoval(force)) return;

    // reset model policy
    adapter->setModelPolicy(DummyModel);

    // check if this graph is still used by the parent graph
    if (auto* p = qobject_cast<Graph const*>(parent()))
    {
        if (p->findModelAdapter()) return;
    }

    delete adapter;

    for (auto* graph : graphNodes())
    {
        graph->clearModelAdapter(false);
    }
}

void
Graph::onObjectDataMerged()
{
    if (auto adapter = findModelAdapter())
    {
        adapter->mergeGraphModel(*this);
    }
}

bool
Graph::triggerEvaluation(PortIndex idx)
{
    if (!isActive()) return false;

    if (auto* input = inputProvider())
    {
        if (idx != invalid<PortIndex>())
        {
            input->setOutData(idx, inData(idx));
            return true;
        }

        auto size = ports(PortType::In).size();
        for (idx = PortIndex{0}; idx < size; ++idx)
        {
            input->setOutData(idx, inData(idx));
        }
        return true;
    }

    return false;
}

void
Graph::initInputOutputProviders()
{
    auto* exstInput = findDirectChild<GroupInputProvider*>();
    auto input = exstInput ? nullptr : std::make_unique<GroupInputProvider>();
    
    auto* exstOutput = findDirectChild<GroupOutputProvider*>();
    auto output = exstOutput ? nullptr : std::make_unique<GroupOutputProvider>();

    if (!exstOutput) exstOutput = output.get();
    connect(exstOutput, &Node::inputDataRecieved,
            this, &Graph::forwardOutData, Qt::UniqueConnection);

    if (input) appendNode(std::move(input));
    if (output) appendNode(std::move(output));
}

void
Graph::forwardOutData(PortIndex idx)
{
    if (auto* output = outputProvider())
    {
        setOutData(idx, output->inData(idx));
    }
}

bool
intelli::evaluate(Graph& graph)
{
    if (graph.findModelAdapter())
    {
        gtError() << QObject::tr("Cannot evaluate a graph with an active evaluation model!");
        return false;
    }

    auto const allNodes = graph.nodes();
    auto allConnections = graph.connections();

    std::map<NodeId, std::vector<NodeId>> callGraph;
    std::map<NodeId, std::vector<Connection*>> connectionGraph;

    std::vector<NodeId> nextNodes;
    std::vector<Connection*> nodeConnections;

    for (auto const* node : allNodes)
    {
        nextNodes.clear();
        nodeConnections.clear();

        NodeId nodeId = node->id();

        for (auto* connection : allConnections)
        {
            if (connection->outNodeId() == nodeId)
            {
                nodeConnections.push_back(connection);
                nextNodes.push_back(connection->inNodeId());
            }
        }

        callGraph.insert(std::make_pair(nodeId, nextNodes));
        connectionGraph.insert(std::make_pair(nodeId, nodeConnections));
    }

    gtDebug().verbose() << "call graph: " << callGraph;

    std::vector<NodeId> callOrder = gt::topo_sort(callGraph);

    gtDebug().verbose() << "call order: " << callOrder;

    // evaluate each node
    for (NodeId nodeId : callOrder)
    {
        auto* node = graph.findNode(nodeId);
        assert(node);

        if (auto* group = qobject_cast<Graph*>(node))
        {
            intelli::evaluate(*group);
        }
        else
        {
            // set executor
            node->setExecutor(ExecutionMode::Sequential);
            // evaluate
            node->updateNode();
            // clear executor
            node->setExecutor(ExecutionMode::None);
        }

        // propagate data to next nodes
        for (auto* connection : connectionGraph.at(nodeId))
        {
            auto* next = graph.findNode(connection->inNodeId());
            if (!next)
            {
                gtError()
                    << QObject::tr("Cannot propagte data from node %1 to node %2!")
                           .arg(node->id()).arg(connection->inNodeId())
                    << QObject::tr("(Node was not found)");
                return false;
            }

            if (!next->setInData(connection->inPortIdx(), node->outData(connection->outPortIdx())))
            {
                gtError()
                    << QObject::tr("Cannot propagte data from node %1 to node %2!")
                           .arg(node->id()).arg(connection->inNodeId())
                    << QObject::tr("(Port index %1 of node %2 out of bounds)")
                           .arg(connection->inPortIdx()).arg(connection->inNodeId());
                return false;
            }
        }
    }

    return true;
}

GtMdiItem*
intelli::show(Graph& graph)
{
    return gtMdiLauncher->open(GT_CLASSNAME(GraphEditor), &graph);
}

GtMdiItem*
intelli::show(std::unique_ptr<Graph> graph)
{
    if (!graph) return nullptr;

    auto* item = gtMdiLauncher->open(GT_CLASSNAME(GraphEditor), graph.get());
    if (!item)
    {
        gtWarning() << QObject::tr("Failed to open Graph Editor for intelli graph")
                    << graph->caption();
        return nullptr;
    }

    assert(item->parent() == item->widget());
    assert(item->parent() != nullptr);

    graph.release()->setParent(item);
    return item;
}
