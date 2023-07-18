/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphmodelmanager.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_coredatamodel.h"

inline gt::log::Stream& operator<<(gt::log::Stream& s, QtNodes::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeConnection["
            << con.inNodeId  << ":" << con.inPortIndex << "/"
            << con.outNodeId << ":" << con.outPortIndex << "]";
    }
    return s;
}

GtIntelliGraphModelManager::GtIntelliGraphModelManager(GtIntelliGraph& parent,
                                                       gt::ig::ModelPolicy policy) :
    m_policy(policy),
    m_graphModel(gt::ig::make_volatile<QtNodes::DataFlowGraphModel>(
        GtIntelliGraphNodeFactory::instance().makeRegistry()
    ))
{
    setObjectName("__model");
    setParent(&parent);

    auto* ig = &parent;

    connect(ig, &GtIntelliGraph::nodeAppended,
            this, &GtIntelliGraphModelManager::appendNodeToModel);
    connect(ig, &GtIntelliGraph::connectionAppended,
            this, &GtIntelliGraphModelManager::appendConnectionToModel);
    connect(ig, &GtIntelliGraph::nodePositionChanged,
            this, [m = m_graphModel.get()](gt::ig::NodeId nodeId, QPointF pos){
        m->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    for (auto* graph : ig->subGraphs())
    {
        graph->initGroupProviders();
        graph->makeModelManager(gt::ig::DummyModel);
    }

    for (auto* node : ig->nodes())
    {
        node->setActive(true);
        appendNodeToModel(node);
    }

    for (auto* con : ig->connections())
    {
        appendConnectionToModel(con);
    }

    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
            this, [=](QtNodes::NodeId nodeId){ appendNodeFromModel(nodeId); });
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, [=](QtNodes::NodeId nodeId){ ig->deleteNode(nodeId); });
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
            this, [=](QtNodes::ConnectionId conId){ appendConnectionFromModel(conId); });
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, [=](QtNodes::ConnectionId conId){ ig->deleteConnection(conId); });

    // once loaded remove all orphan nodes and connections
//    removeOrphans(*ig);
}

gt::ig::ModelPolicy
GtIntelliGraphModelManager::policy() const
{
    return m_policy;
}

void
GtIntelliGraphModelManager::updatePolicy(gt::ig::ModelPolicy policy)
{
    // upgrade dummy model to active model
    if (policy == gt::ig::ActiveModel)
    {
        m_policy = policy;
    }
}

GtIntelliGraph*
GtIntelliGraphModelManager::intelliGraph()
{
    return qobject_cast<GtIntelliGraph*>(parent());
}

const GtIntelliGraph*
GtIntelliGraphModelManager::intelliGraph() const
{
    return const_cast<GtIntelliGraphModelManager*>(this)->intelliGraph();
}

bool
GtIntelliGraphModelManager::readyForRemoval(bool force) const
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << QObject::tr("Null intelli graph!");
        return true;
    }

    // dont close an active model if we are not forcing it
    if (m_policy != gt::ig::DummyModel && !force) return false;

    // reset policy
    m_policy = gt::ig::DummyModel;

    // check if this graph is still used by the parent graph
    if (auto* parent = qobject_cast<GtIntelliGraph const*>(ig->parent()))
    {
        if (parent->findModelManager()) return false;
    }

    return true;
}

void
GtIntelliGraphModelManager::mergeGraph(GtIntelliGraph& ig)
{
    // after undo/redo we may have to add resoted nodes and connections to
    // the model
    auto const& nodes = ig.nodes();
    auto const& connections = ig.connections();

    auto const& modelNodes = m_graphModel->allNodeIds();

    for (auto* node : nodes)
    {
        if (modelNodes.find(node->id()) == modelNodes.end())
        {
            gtDebug().verbose().nospace()
                << "### Merging node " << node->modelName()
                << "(" << node->id() << ")";

            // update graph model
            appendNodeToModel(node);
        }

        auto const& modelConnections = m_graphModel->allConnectionIds(node->id());

        // find connections that belong to node
        std::vector<GtIntelliGraphConnection*> nodeConnections;
        std::copy_if(std::cbegin(connections), std::cend(connections),
                     std::back_inserter(nodeConnections),
                     [id = node->id()](auto const* con){
                         return con->inNodeId() == id || con->outNodeId() == id;
                     });

        for (auto* con : qAsConst(nodeConnections))
        {
            auto conId = con->toConnectionId();
            if (modelConnections.find(conId) == modelConnections.end())
            {
                gtDebug().verbose().nospace()
                    << "### Merging connection " << conId;
                // update graph model
                appendConnectionToModel(con);
            }
        }
    }
}

QtNodes::DataFlowGraphModel*
GtIntelliGraphModelManager::graphModel()
{
    return m_graphModel.get();
}

QtNodes::DataFlowGraphModel const*
GtIntelliGraphModelManager::graphModel() const
{
    return m_graphModel.get();
}

bool
GtIntelliGraphModelManager::appendNodeFromModel(QtNodes::NodeId nodeId)
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << QObject::tr("Null intelli graph!");
        return false;
    }

    auto* model = m_graphModel->delegateModel<GtIntelliGraphObjectModel>(nodeId);
    if (!model)
    {
        gtWarning().medium() << tr("Unkown model for node '%1'!").arg(nodeId);
        return false;
    }

    if (GtIntelliGraphNode* node = ig->findNode(nodeId))
    {
        gtWarning().medium() << tr("Node '%1' already exists!").arg(nodeId);
        return false;
    }

    // move node from model to object tree
    auto node = std::unique_ptr<GtIntelliGraphNode>(model->node());
    if (!node)
    {
        gtWarning() << tr("Node for model '%1' is null!").arg(nodeId);
        return false;
    }

    node->setId(gt::ig::NodeId{nodeId});

    gtInfo().medium() << tr("Appending node: %1 (id: %2)")
                         .arg(node->objectName()).arg(nodeId);

    if (auto* n = ig->appendNode(std::move(node)))
    {
        n->updateObjectName();
        n->updateNode();
        setupNode(*n);
        return true;
    }

    gtError() << tr("Failed to append node '%1'").arg(node->objectName());
    m_graphModel->deleteNode(nodeId);
    return false;
}

bool
GtIntelliGraphModelManager::appendConnectionFromModel(QtNodes::ConnectionId connectionId)
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << QObject::tr("Null intelli graph!");
        return false;
    }

    if (auto* connection = ig->findConnection(connectionId))
    {
        gtWarning().medium()
            << tr("Connection was already created!") << connectionId;
        return false;
    }

    gtInfo().medium() << tr("Appending connection:") << connectionId;

    auto connection = std::make_unique<GtIntelliGraphConnection>();
    connection->fromConnectionId(connectionId);

    if (auto* con = ig->appendConnection(std::move(connection)))
    {
        setupConnection(*connection);
        return true;
    }

    gtError() << tr("Failed to append connection") << connectionId;
    return false;
}

bool
GtIntelliGraphModelManager::appendNodeToModel(GtIntelliGraphNode* node)
{
    if (!node) return false;

    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << QObject::tr("Null intelli graph!");
        return false;
    }

    QtNodes::NodeId oldId = node->id();

    // add delegate model
    auto model = std::make_unique<GtIntelliGraphObjectModel>(*node);

    QtNodes::NodeId newId = m_graphModel->addNode(std::move(model), oldId);
    if (newId == gt::ig::invalid<gt::ig::NodeId>())
    {
        gtError() << tr("Failed to add node %1 to graph model! Error:")
                         .arg(node->modelName());
        return false;
    }

    emit m_graphModel->nodeCreated(node->id());

    // update node id if necessary
    if (newId != oldId) node->setId(gt::ig::NodeId::fromValue(newId));

    // update node position
    ig->setNodePosition(newId, node->pos());

    setupNode(*node);

    return true;
}

bool
GtIntelliGraphModelManager::appendConnectionToModel(GtIntelliGraphConnection* connection)
{
    if (!connection) return false;

    m_graphModel->addConnection(connection->toConnectionId());

    setupConnection(*connection);

    return true;
}

void
GtIntelliGraphModelManager::removeOrphans(GtIntelliGraph& ig)
{
    auto nodes = ig.nodes();
    auto cons = ig.connections();

    for (auto nodeId : m_graphModel->allNodeIds())
    {
        foreach (auto* node, qAsConst(nodes))
        {
            if (auto* m = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId))
            {
                if (node->id() == nodeId && m->name() == node->modelName())
                {
                    nodes.removeOne(node);
                }
            }
        }

        for (auto conId : m_graphModel->allConnectionIds(nodeId))
        {
            foreach (auto* con, qAsConst(cons))
            {
                if (con->toConnectionId() == conId) cons.removeOne(con);
            }
        }
    }

    GtObjectList objects;
    objects.reserve(cons.size() + nodes.size());
    std::copy(std::cbegin(cons), std::cend(cons), std::back_inserter(objects));
    std::copy(std::cbegin(nodes), std::cend(nodes), std::back_inserter(objects));

    if (!objects.empty()) gtDataModel->deleteFromModel(objects);
}

void
GtIntelliGraphModelManager::setupNode(GtIntelliGraphNode& node)
{
    connect(&node, &QObject::destroyed, m_graphModel.get(),
            [graph = m_graphModel.get(),
             model = node.modelName(),
             nodeId = node.id()](){
        gtDebug().verbose() << "Deleting node from model:" << model
                            << "(" << nodeId << ")";;
        graph->deleteNode(nodeId);
    });
    connect(&node, &GtIntelliGraphNode::nodeChanged, m_graphModel.get(),
            [graph = m_graphModel.get(),
             nodeId = node.id()](){
        emit graph->nodeUpdated(nodeId);
    });

    bool isActive = graphModel();

    // init input output providers
    if (auto group = qobject_cast<GtIntelliGraph*>(&node))
    {
        group->initGroupProviders();

        // initialize graph model if active
        if (isActive)
        {
            group->makeModelManager(gt::ig::DummyModel);
        }
    }

    node.setActive(isActive);
}

void
GtIntelliGraphModelManager::setupConnection(GtIntelliGraphConnection& connection)
{
    connect(&connection, &QObject::destroyed,m_graphModel.get(),
            [graph = m_graphModel.get(),
             conId = connection.toConnectionId()](){
        gtDebug().verbose() << "Deleting connection from model:" << conId;
        graph->deleteConnection(conId);
    });
}

