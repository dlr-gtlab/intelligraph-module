/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphmodeladapter.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_coredatamodel.h"

inline gt::log::Stream&
operator<<(gt::log::Stream& s, QtNodes::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeConnection["
            << con.outNodeId << ":" << con.outPortIndex << "/"
            << con.inNodeId  << ":" << con.inPortIndex  << "]";
    }
    return s;
}

GtIntelliGraphModelAdapter::GtIntelliGraphModelAdapter(GtIntelliGraph& parent,
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
            this, &GtIntelliGraphModelAdapter::appendNodeToModel);
    connect(ig, &GtIntelliGraph::connectionAppended,
            this, &GtIntelliGraphModelAdapter::appendConnectionToModel);
    connect(ig, &GtIntelliGraph::nodePositionChanged,
            this, [m = m_graphModel.get()](gt::ig::NodeId nodeId, QPointF pos){
        m->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    // setup all other subgraphs
    for (auto* graph : ig->subGraphs())
    {
        graph->initGroupProviders();
        graph->makeModelAdapter(gt::ig::DummyModel);
    }

    // merge all existing nodes and connections
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
            this, &GtIntelliGraphModelAdapter::appendNodeFromModel);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, [=](QtNodes::NodeId nodeId){ ig->deleteNode(nodeId); });
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
            this, &GtIntelliGraphModelAdapter::appendConnectionFromModel);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, [=](QtNodes::ConnectionId conId){ ig->deleteConnection(conId); });
}

GtIntelliGraphModelAdapter::~GtIntelliGraphModelAdapter()
{
    auto ig = intelliGraph();
    if (!ig) return;

    for (auto* node : ig->nodes())
    {
        node->setActive(false);
    }
}

GtIntelliGraph*
GtIntelliGraphModelAdapter::intelliGraph()
{
    return qobject_cast<GtIntelliGraph*>(parent());
}

const GtIntelliGraph*
GtIntelliGraphModelAdapter::intelliGraph() const
{
    return const_cast<GtIntelliGraphModelAdapter*>(this)->intelliGraph();
}

QtNodes::DataFlowGraphModel*
GtIntelliGraphModelAdapter::graphModel()
{
    return m_graphModel.get();
}

QtNodes::DataFlowGraphModel const*
GtIntelliGraphModelAdapter::graphModel() const
{
    return const_cast<GtIntelliGraphModelAdapter*>(this)->graphModel();
}

bool
GtIntelliGraphModelAdapter::readyForRemoval(bool force) const
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << tr("Null intelli graph!");
        return true;
    }

    // dont close an active model if we are not forcing it
    return m_policy == gt::ig::DummyModel || force;
}

void
GtIntelliGraphModelAdapter::mergeConnections(GtIntelliGraph& ig)
{
    gtDebug().medium() << __FUNCTION__ << ig.objectName();

    auto const& connections = ig.connections();

    for (auto* con : connections)
    {
        if (m_graphModel->nodeExists(con->outNodeId()) &&
            m_graphModel->nodeExists(con->inNodeId()))
        {
            appendConnectionToModel(con);
        }
    }
}

void
GtIntelliGraphModelAdapter::mergeGraphModel(GtIntelliGraph& ig)
{
    gtDebug().medium() << __FUNCTION__ << ig.objectName();

    // after undo/redo we may have to add resoted nodes and connections to
    // the model
    auto const& nodes = ig.nodes();
    auto const& connections = ig.connections();

    auto modelNodes = m_graphModel->allNodeIds();

    for (auto* node : nodes)
    {
        auto nodeIter = modelNodes.find(node->id());
        // node not yet in model
        if (nodeIter == modelNodes.end())
        {
            gtDebug().verbose().nospace()
                << "## Merging node '" << node->objectName()
                << "' (" << node->id() << ")";

            // update graph model
            appendNodeToModel(node);
        }
        // node in model and graph
        else modelNodes.erase(nodeIter);

        auto const& modelConnections = m_graphModel->allConnectionIds(node->id());

        // find connections that belong to the node
        std::vector<GtIntelliGraphConnection*> nodeConnections;
        std::copy_if(std::cbegin(connections), std::cend(connections),
                     std::back_inserter(nodeConnections),
                     [id = node->id(), m = m_graphModel.get()](auto const* con){
            assert(m->nodeExists(con->inNodeId()));
            assert(m->nodeExists(con->outNodeId()));
            return con->inNodeId()  == id || con->outNodeId() == id;
        });

        for (auto* con : qAsConst(nodeConnections))
        {
            auto conId = con->toConnectionId();
            if (modelConnections.find(conId) == modelConnections.end())
            {
                gtDebug().verbose().nospace()
                    << "## Merging connection " << conId;
                // update graph model
                appendConnectionToModel(con);
            }
        }
    }

    if (!modelNodes.empty())
    {
        gtWarning() << "Unkown nodes:" << modelNodes;
    }
}

bool
GtIntelliGraphModelAdapter::appendNodeFromModel(QtNodes::NodeId nodeId)
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << tr("Null intelli graph!");
        return false;
    }

    auto* model = m_graphModel->delegateModel<GtIntelliGraphObjectModel>(nodeId);
    if (!model)
    {
        gtWarning() << tr("Unkown model for node '%1'!").arg(nodeId);
        return false;
    }

    if (GtIntelliGraphNode* node = ig->findNode(nodeId))
    {
        gtWarning().verbose() << tr("Node '%1' already exists!").arg(nodeId);
        return false;
    }

    // move node from model to object tree
    auto node = std::unique_ptr<GtIntelliGraphNode>(model->node());
    if (!node)
    {
        gtError() << tr("Node for model '%1' is null!").arg(nodeId);
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
GtIntelliGraphModelAdapter::appendConnectionFromModel(QtNodes::ConnectionId connectionId)
{
    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << tr("Null intelli graph!");
        return false;
    }

    if (auto* connection = ig->findConnection(connectionId))
    {
        gtWarning().verbose()
            << tr("Connection was already created!") << connectionId;
        return false;
    }

    gtInfo().medium() << tr("Appending connection:") << connectionId;

    if (auto* con = ig->appendConnection(std::make_unique<GtIntelliGraphConnection>(connectionId)))
    {
        setupConnection(*con);
        return true;
    }

    gtError() << tr("Failed to append connection") << connectionId;
    return false;
}

bool
GtIntelliGraphModelAdapter::appendNodeToModel(GtIntelliGraphNode* node)
{
    if (!node) return false;

    auto* ig = intelliGraph();
    if (!ig)
    {
        gtError() << tr("Null intelli graph!");
        return false;
    }

    // node may already exists
    if (m_graphModel->nodeExists(node->id())) return false;

    // add delegate model
    auto model = std::make_unique<GtIntelliGraphObjectModel>(*node);

    if (m_graphModel->addNode(std::move(model), node->id()) == QtNodes::InvalidNodeId)
    {
        gtError() << tr("Failed to add node %1 to graph model! Error:")
                     .arg(node->modelName());
        return false;
    }

    emit m_graphModel->nodeCreated(node->id());

    // update node position
    ig->setNodePosition(node->id(), node->pos());

    setupNode(*node);

    return true;
}

bool
GtIntelliGraphModelAdapter::appendConnectionToModel(GtIntelliGraphConnection* connection)
{
    if (!connection) return false;

    auto conId = connection->toConnectionId();

    // connnection may already exist
    if (m_graphModel->connectionExists(conId)) return false;

    m_graphModel->addConnection(conId);

    setupConnection(*connection);

    return true;
}

void
GtIntelliGraphModelAdapter::removeOrphans(GtIntelliGraph& ig)
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
GtIntelliGraphModelAdapter::setupNode(GtIntelliGraphNode& node)
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

    // init input output providers
    if (auto group = qobject_cast<GtIntelliGraph*>(&node))
    {
        group->initGroupProviders();
        group->makeModelAdapter(gt::ig::DummyModel);
    }

    node.setActive(true);
}

void
GtIntelliGraphModelAdapter::setupConnection(GtIntelliGraphConnection& connection)
{
    connect(&connection, &QObject::destroyed,m_graphModel.get(),
            [graph = m_graphModel.get(),
             conId = connection.toConnectionId()](){
        gtDebug().verbose() << "Deleting connection from model:" << conId;
        graph->deleteConnection(conId);
    });
}

