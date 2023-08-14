/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/adapter/modeladapter.h"

#include "intelli/private/utils.h"
#include "intelli/exec/executorfactory.h"
#include "intelli/graph.h"
#include "intelli/node.h"
#include "intelli/connection.h"
#include "intelli/nodefactory.h"

#include "intelli/adapter/objectmodel.h"

#include "gt_coreapplication.h"
#include "gt_command.h"

using namespace intelli;

ModelAdapter::ModelAdapter(Graph& parent,
                           ModelPolicy policy) :
    m_policy(policy),
    m_graphModel(make_volatile<QtNodes::DataFlowGraphModel>(
        NodeFactory::instance().makeRegistry()
    ))
{
    setObjectName("__model");
    setParent(&parent);

    auto& ig = parent;
    
    connect(&ig, &Graph::nodeAppended,
            this, &ModelAdapter::appendNodeToModel, Qt::UniqueConnection);
    connect(&ig, &Graph::connectionAppended,
            this, &ModelAdapter::appendConnectionToModel, Qt::UniqueConnection);
    connect(&ig, &Graph::nodePositionChanged,
            this, [m = m_graphModel.get()](NodeId nodeId, QPointF pos){
        m->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    // setup all other graph nodes
    for (auto* graph : ig.graphNodes())
    {
        graph->initInputOutputProviders();
        graph->makeModelAdapter(DummyModel);
    }

    // merge all existing nodes and connections
    for (auto* node : ig.nodes())
    {
        appendNodeToModel(node);
    }

    for (auto* con : ig.connections())
    {
        appendConnectionToModel(con);
    }

    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
            this, &ModelAdapter::appendNodeFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, &ModelAdapter::onNodeDeletedFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
            this, &ModelAdapter::appendConnectionFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, &ModelAdapter::onConnectionDeletedFromModel, Qt::UniqueConnection);
}

ModelAdapter::~ModelAdapter()
{
    for (auto* node : intelliGraph().nodes())
    {
        node->setExecutor(ExecutionMode::None);
    }
}

Graph&
ModelAdapter::intelliGraph()
{
    auto tmp = qobject_cast<Graph*>(parent());
    assert(tmp);
    return *tmp;
}

const Graph&
ModelAdapter::intelliGraph() const
{
    return const_cast<ModelAdapter*>(this)->intelliGraph();
}

QtNodes::DataFlowGraphModel*
ModelAdapter::graphModel()
{
    return m_graphModel.get();
}

QtNodes::DataFlowGraphModel const*
ModelAdapter::graphModel() const
{
    return const_cast<ModelAdapter*>(this)->graphModel();
}

bool
ModelAdapter::readyForRemoval(bool force) const
{
    // dont close an active model if we are not forcing it
    return m_policy == DummyModel || force;
}

void
ModelAdapter::mergeConnections(Graph& ig)
{
    gtTrace().verbose() << __FUNCTION__ << ig.objectName();

    auto const& connections = ig.connections();

    for (auto* con : connections)
    {
        if (m_graphModel->nodeExists(con->outNodeId()) &&
            m_graphModel->nodeExists(con->inNodeId()) &&
            !m_graphModel->connectionExists(convert(con->connectionId())))
        {
            appendConnectionToModel(con);
        }
    }
}

void
ModelAdapter::mergeGraphModel(Graph& ig)
{
    gtTrace().verbose() << __FUNCTION__ << ig.objectName();

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
        std::vector<Connection*> nodeConnections;
        std::copy_if(std::cbegin(connections), std::cend(connections),
                     std::back_inserter(nodeConnections),
                     [id = node->id(), m = m_graphModel.get()](auto const* con){
            return m->nodeExists(con->inNodeId()) && m->nodeExists(con->outNodeId()) &&
                   (con->inNodeId()  == id || con->outNodeId() == id);
        });

        for (auto* con : qAsConst(nodeConnections))
        {
            auto conId = convert(con->connectionId());
            if (modelConnections.find(conId) == modelConnections.end())
            {
                gtDebug().verbose().nospace()
                    << "## Merging connection " << conId;
                // update graph model
                appendConnectionToModel(con);
            }
        }
    }

    assert (modelNodes.empty());
}

bool
ModelAdapter::appendNodeFromModel(QtNodes::NodeId nodeId)
{
    auto& ig = intelliGraph();

    auto* model = m_graphModel->delegateModel<ObjectModel>(nodeId);
    if (!model)
    {
        gtWarning() << tr("Unkown delegate model! (id: %1)").arg(nodeId);
        return false;
    }

    // move node from model to object tree
    auto node = std::unique_ptr<Node>(model->node());
    if (!node)
    {
        gtError() << tr("Invalid delegate model! (id: %1)").arg(nodeId);
        return false;
    }

    node->setId(NodeId::fromValue(nodeId));

    auto cmd = gtApp->startCommand(&ig, tr("Appending node '%1'").arg(node->objectName()));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    auto ignore = ignoreSignal(
        &ig, &Graph::nodeAppended,
        this, &ModelAdapter::appendNodeToModel
    );

    if (auto* n = ig.appendNode(std::move(node), KeepNodeId))
    {
        setupNode(*n);
        n->updateObjectName();
        n->updateNode();
        return true;
    }

    gtError() << tr("Failed to append node '%1' to graph model!").arg(nodeId);
    m_graphModel->deleteNode(nodeId);
    return false;
}

bool
ModelAdapter::appendConnectionFromModel(QtNodes::ConnectionId conId)
{
    auto& ig = intelliGraph();

    auto cmd = gtApp->startCommand(&ig, tr("Appending connection '%1:%2/%3:%4'")
                                            .arg(conId.outNodeId)
                                            .arg(conId.outPortIndex)
                                            .arg(conId.inNodeId)
                                            .arg(conId.inPortIndex));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    auto ignore = ignoreSignal(
        &ig, &Graph::connectionAppended,
        this, &ModelAdapter::appendConnectionToModel
    );
    
    if (auto* con = ig.appendConnection(std::make_unique<Connection>(convert(conId))))
    {
        setupConnection(*con);
        return true;
    }

    gtError() << tr("Failed to append connection to graph model!")
              << conId;
    return false;
}

bool
ModelAdapter::appendNodeToModel(Node* node)
{
    if (!node) return false;

    auto& ig = intelliGraph();

    auto nodeId = node->id();

    // node may already exists
    if (m_graphModel->nodeExists(nodeId))
    {
        gtWarning() << tr("Node '%1' already exists in the graph model!")
                           .arg(node->id());
        return false;
    }

    gtInfo().verbose() << tr("Appending node to graph model: %1 (id: %2) ")
                              .arg(node->objectName()).arg(nodeId);

    // add delegate model
    auto model = std::make_unique<ObjectModel>(*node);

    if (m_graphModel->addNode(std::move(model), nodeId) == QtNodes::InvalidNodeId)
    {
        gtError() << tr("Failed to add node %1 to the graph model! Error:")
                         .arg(node->modelName());
        return false;
    }

    {
        auto ignore = ignoreSignal(
            m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
            this, &ModelAdapter::appendNodeFromModel
        );

        emit m_graphModel->nodeCreated(nodeId);
    }

    // update node position
    ig.setNodePosition(node, node->pos());

    setupNode(*node);

    return true;
}

bool
ModelAdapter::appendConnectionToModel(Connection* connection)
{
    if (!connection) return false;

    auto conId = convert(connection->connectionId());

    // connnection may already exist
    if (m_graphModel->connectionExists(conId))
    {
        gtWarning() << tr("Connection '%1' already exists in the graph model!")
                           .arg(connection->objectName());
        return false;
    }

    gtInfo().verbose() << tr("Appending connection to graph model:") << conId;

    {
        auto ignore = ignoreSignal(
            m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
            this, &ModelAdapter::appendConnectionFromModel
        );

        m_graphModel->addConnection(conId);
    }

    setupConnection(*connection);

    return true;
}

void
ModelAdapter::removeOrphans(Graph& ig)
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
                if (convert(con->connectionId()) == conId) cons.removeOne(con);
            }
        }
    }

    qDeleteAll(cons);
    qDeleteAll(nodes);
}

void
ModelAdapter::setupNode(Node& node)
{
    connect(&node, &QObject::destroyed, m_graphModel.get(),
            [model = m_graphModel.get(),
             name = node.modelName(),
             nodeId = node.id(),
             this](){
        auto ignore = ignoreSignal(
            model, &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, &ModelAdapter::onNodeDeletedFromModel
        );
        gtDebug().verbose() << "Deleting node from model:" << name
                            << "(" << nodeId << ")";;
        model->deleteNode(nodeId);
    });
    
    auto updateGraphics = [model = m_graphModel.get(),
                           nodeId = node.id()](){
        emit model->nodeUpdated(nodeId);
    };

    connect(&node, &Node::nodeChanged, m_graphModel.get(), updateGraphics);
    connect(&node, &Node::portChanged, m_graphModel.get(), updateGraphics);

    auto updateNodeState = [model = m_graphModel.get(),
                            nodeId = node.id()](){
        emit model->nodeEvalStateUpdated(nodeId);
    };

    connect(&node, &Node::nodeStateChanged,
            m_graphModel.get(), updateNodeState);
    connect(&node, &Node::computingStarted,
            m_graphModel.get(), updateNodeState);
    connect(&node, &Node::computingFinished,
            m_graphModel.get(), updateNodeState);

    // init input output providers
    if (auto group = qobject_cast<Graph*>(&node))
    {
        group->makeModelAdapter(DummyModel);
    }

    node.setExecutor(ExecutionMode::Default);
}

void
ModelAdapter::setupConnection(Connection& connection)
{
    connect(&connection, &QObject::destroyed,m_graphModel.get(),
            [model = m_graphModel.get(),
             conId = convert(connection.connectionId()),
             this](){
        auto ignore = ignoreSignal(
            model, &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, &ModelAdapter::onConnectionDeletedFromModel
        );
        gtDebug().verbose() << "Deleting connection from model:" << conId;
        model->deleteConnection(conId);
    });
}

void
ModelAdapter::onNodeDeletedFromModel(QtNodes::NodeId nodeId)
{
    auto& ig = intelliGraph();

    auto cmd = gtApp->startCommand(&ig, tr("Deleting node '%1'").arg(nodeId));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    ig.deleteNode(NodeId::fromValue(nodeId));
}

void
ModelAdapter::onConnectionDeletedFromModel(QtNodes::ConnectionId conId)
{
    auto& ig = intelliGraph();

    auto cmd = gtApp->startCommand(&ig, tr("Deleting connection '%1:%2/%3:%4'")
                                            .arg(conId.outNodeId)
                                            .arg(conId.outPortIndex)
                                            .arg(conId.inNodeId)
                                            .arg(conId.inPortIndex));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    ig.deleteConnection(convert(conId));
}

