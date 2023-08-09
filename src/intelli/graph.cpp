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
#include "intelli/private/utils.h"

#include "gt_qtutilities.h"

#include <QThread>
#include <QCoreApplication>

using namespace intelli;

GTIG_REGISTER_NODE(Graph, "Group")

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
                        QtNodes::ConnectionId conId)
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
    Node("Sub Graph")
{
    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* group = new ConnectionGroup(this);
    group->setDefault(true);
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
Graph::subGraphs()
{
    return findDirectChildren<Graph*>();
}

QList<Graph const*>
Graph::subGraphs() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->subGraphs()
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

inline auto
makeTemporaryModelAdapter(Graph* this_)
{
    auto finally = gt::finally([=](){
        this_->clearModelAdapter(false);
    });

    if (!this_->findModelAdapter())
    {
        this_->makeModelAdapter(DummyModel);
        return finally;
    }

    finally.clear();
    return finally;
}

Node::NodeDataPtr
Graph::eval(PortId outId)
{
    auto out = outputProvider();
    if (!out)
    {
        gtError().medium() << tr("Failed to evaluate group node! (Invalid output provider)");
        return {};
    };

    auto in = inputProvider();
    if (!in)
    {
        gtError().medium() << tr("Failed to evaluate group node! (Invalid input provider)");
        return {};
    }

    // make sure a model exist and i cleaned up if needed
    auto cleanup = makeTemporaryModelAdapter(this);
    Q_UNUSED(cleanup);

    // force subnodes to use a sequential execution
    for (auto* node : nodes())
    {
        node->setExecutor(ExecutorMode::Sequential);
    }

    // this will trigger the evaluation
    in->updateNode();

    return nodeData(outId);
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
        if (!obj) return {};

        auto oldId = obj->id();

        auto* node = appendNode(std::move(obj));
        if (!node) return {};

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
        if (!con) return {};
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

GtIntelliGraphModelAdapter*
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

    return new GtIntelliGraphModelAdapter(*this, policy);
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

    for (auto* graph : subGraphs())
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

void
Graph::initGroupProviders()
{
    auto* exstInput = findDirectChild<GroupInputProvider*>();
    auto input = exstInput ? nullptr : std::make_unique<GroupInputProvider>();
    
    auto* exstOutput = findDirectChild<GroupOutputProvider*>();
    auto output = exstOutput ? nullptr : std::make_unique<GroupOutputProvider>();

    appendNode(std::move(output));
    appendNode(std::move(input));
}
