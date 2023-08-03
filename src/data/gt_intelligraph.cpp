/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraph.h"

#include "gt_intelligraphconnection.h"
#include "gt_intelligraphconnectiongroup.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraphmodeladapter.h"
#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"
#include "private/utils.h"

#include "gt_qtutilities.h"

#include <QThread>
#include <QCoreApplication>

GTIG_REGISTER_NODE(GtIntelliGraph, "Group")

template <typename ObjectList, typename T = gt::trait::value_t<ObjectList>>
inline T findNode(ObjectList const& nodes,
                  QtNodes::NodeId nodeId)
{
    auto iter = std::find_if(std::begin(nodes), std::end(nodes),
                             [=](GtIntelliGraphNode const* node) {
        return node->id() == nodeId;
    });

    return iter == std::end(nodes) ? nullptr : *iter;
}

template <typename ObjectList, typename T = gt::trait::value_t<ObjectList>>
inline T findConnection(ObjectList const& connections,
                        QtNodes::ConnectionId conId)
{
    auto iter = std::find_if(std::begin(connections), std::end(connections),
                             [&](GtIntelliGraphConnection* connection){
        return conId == connection->connectionId();
    });

    return iter == std::end(connections) ? nullptr : *iter;
}

/// checks and updates the node id of the node depending of the policy specified
bool
updateNodeId(GtIntelliGraph const& graph, GtIntelliGraphNode& node, gt::ig::NodeIdPolicy policy)
{
    using gt::ig::NodeId;

    auto const nodes = graph.nodes();

    // id may already be used
    QVector<NodeId> ids;
    ids.reserve(nodes.size());
    std::transform(std::begin(nodes), std::end(nodes), std::back_inserter(ids),
                   [](GtIntelliGraphNode const* n){ return n->id(); });

    if (ids.contains(node.id()))
    {
        if (policy != gt::ig::UpdateNodeId) return false;

        // generate a new one
        auto maxId = *std::max_element(std::begin(ids), std::end(ids)) + 1;
        node.setId(gt::ig::NodeId::fromValue(maxId));
        assert(node.id() != gt::ig::invalid<NodeId>());
        return true;
    }
    return true;
}

GtIntelliGraph::GtIntelliGraph() :
    GtIntelliGraphNode("Sub Graph")
{
    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* group = new GtIntellIGraphConnectionGroup(this);
    group->setDefault(true);
}

GtIntelliGraph::~GtIntelliGraph()
{
    // we should stop the execution of the model manager first
    // (if its still active)
    delete findModelAdapter();
}

QList<GtIntelliGraphNode*>
GtIntelliGraph::nodes()
{
    return findDirectChildren<GtIntelliGraphNode*>();
}

QList<GtIntelliGraphNode const*>
GtIntelliGraph::nodes() const
{
    return gt::container_const_cast(
        const_cast<GtIntelliGraph*>(this)->nodes()
    );
}

QList<GtIntelliGraphConnection*>
GtIntelliGraph::connections()
{
    return connectionGroup().findDirectChildren<GtIntelliGraphConnection*>();
}

QList<GtIntelliGraphConnection const*>
GtIntelliGraph::connections() const
{
    return gt::container_const_cast(
        const_cast<GtIntelliGraph*>(this)->connections()
    );
}

GtIntellIGraphConnectionGroup&
GtIntelliGraph::connectionGroup()
{
    auto* group = findDirectChild<GtIntellIGraphConnectionGroup*>();
    assert(group);
    return *group;
}

GtIntellIGraphConnectionGroup const& GtIntelliGraph::connectionGroup() const
{
    return const_cast<GtIntelliGraph*>(this)->connectionGroup();
}

GtIgGroupInputProvider*
GtIntelliGraph::inputProvider()
{
    return findDirectChild<GtIgGroupInputProvider*>();
}

GtIgGroupInputProvider const*
GtIntelliGraph::inputProvider() const
{
    return const_cast<GtIntelliGraph*>(this)->inputProvider();
}

GtIgGroupOutputProvider*
GtIntelliGraph::outputProvider()
{
    return this->findDirectChild<GtIgGroupOutputProvider*>();
}

GtIgGroupOutputProvider const*
GtIntelliGraph::outputProvider() const
{
    return const_cast<GtIntelliGraph*>(this)->outputProvider();
}

GtIntelliGraphNode*
GtIntelliGraph::findNode(NodeId nodeId)
{
    return ::findNode(nodes(), nodeId);
}

GtIntelliGraphNode const*
GtIntelliGraph::findNode(NodeId nodeId) const
{
    return const_cast<GtIntelliGraph*>(this)->findNode(nodeId);
}

GtIntelliGraphConnection*
GtIntelliGraph::findConnection(ConnectionId const& conId)
{
    return ::findConnection(connections(), conId);
}

GtIntelliGraphConnection const*
GtIntelliGraph::findConnection(ConnectionId const& conId) const
{
    return const_cast<GtIntelliGraph*>(this)->findConnection(conId);
}

QList<GtIntelliGraph*>
GtIntelliGraph::subGraphs()
{
    return findDirectChildren<GtIntelliGraph*>();
}

QList<GtIntelliGraph const*>
GtIntelliGraph::subGraphs() const
{
    return gt::container_const_cast(
        const_cast<GtIntelliGraph*>(this)->subGraphs()
    );
}

GtIntelliGraphModelAdapter*
GtIntelliGraph::findModelAdapter()
{
    return findDirectChild<GtIntelliGraphModelAdapter*>();
}

GtIntelliGraphModelAdapter const*
GtIntelliGraph::findModelAdapter() const
{
    return const_cast<GtIntelliGraph*>(this)->findModelAdapter();
}

inline auto
makeTemporaryModelAdapter(GtIntelliGraph* this_)
{
    auto finally = gt::finally([=](){
        this_->clearModelAdapter(false);
    });

    if (!this_->findModelAdapter())
    {
        this_->makeModelAdapter(gt::ig::DummyModel);
        return finally;
    }

    finally.clear();
    return finally;
}

GtIntelliGraphNode::NodeData
GtIntelliGraph::eval(PortId outId)
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
        node->setExecutor(ExecutorType::SequentialExecutor);
    }

    // this will trigger the evaluation
    in->updateNode();

    return nodeData(outId);
}

void
GtIntelliGraph::clear()
{
    qDeleteAll(connections());
    qDeleteAll(nodes());
}

GtIntelliGraphNode*
GtIntelliGraph::appendNode(std::unique_ptr<GtIntelliGraphNode> node, gt::ig::NodeIdPolicy policy)
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

GtIntelliGraphConnection*
GtIntelliGraph::appendConnection(std::unique_ptr<GtIntelliGraphConnection> connection)
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

QVector<gt::ig::NodeId>
GtIntelliGraph::appendObjects(std::vector<std::unique_ptr<GtIntelliGraphNode>>& nodes,
                              std::vector<std::unique_ptr<GtIntelliGraphConnection>>& connections)
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
GtIntelliGraph::deleteNode(NodeId nodeId)
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
GtIntelliGraph::deleteConnection(ConnectionId connectionId)
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
GtIntelliGraph::setNodePosition(GtIntelliGraphNode* node, QPointF pos)
{
    if (node)
    {
        node->setPos(pos);
        emit nodePositionChanged(node->id(), pos);
    }
}

GtIntelliGraphModelAdapter*
GtIntelliGraph::makeModelAdapter(gt::ig::ModelPolicy policy)
{
    if (auto* adapter = findModelAdapter())
    {
        // upgrade dummy model to active model
        if (policy == gt::ig::ActiveModel)
        {
            adapter->setModelPolicy(policy);
        }
        return adapter;
    }

    return new GtIntelliGraphModelAdapter(*this, policy);
}

void
GtIntelliGraph::clearModelAdapter(bool force)
{
    auto* adapter = findModelAdapter();
    if (!adapter)
    {
        gtWarning() << tr("Model adapter not found!");
        return;
    }

    if (!adapter->readyForRemoval(force)) return;

    // reset model policy
    adapter->setModelPolicy(gt::ig::DummyModel);

    // check if this graph is still used by the parent graph
    if (auto* p = qobject_cast<GtIntelliGraph const*>(parent()))
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
GtIntelliGraph::onObjectDataMerged()
{
    if (auto adapter = findModelAdapter())
    {
        adapter->mergeGraphModel(*this);
    }
}

void
GtIntelliGraph::initGroupProviders()
{
    auto* exstInput = findDirectChild<GtIgGroupInputProvider*>();
    auto input = exstInput ? nullptr : std::make_unique<GtIgGroupInputProvider>();

    auto* exstOutput = findDirectChild<GtIgGroupOutputProvider*>();
    auto output = exstOutput ? nullptr : std::make_unique<GtIgGroupOutputProvider>();

    appendNode(std::move(output));
    appendNode(std::move(input));
}
