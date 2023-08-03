/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphmodeladapter.h"

#include "private/utils.h"
#include "gt_intelligraphexecutorfactory.h"
#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_coreapplication.h"
#include "gt_command.h"

template <typename Sender, typename SignalSender,
          typename Reciever, typename SignalReciever>
struct IgnoreSignal
{
    IgnoreSignal(Sender sender_, SignalSender signalSender_,
                 Reciever reciever_, SignalReciever signalReciever_) :
        sender(sender_), signalSender(signalSender_), reciever(reciever_), signalReciever(signalReciever_)
    {
        QObject::disconnect(sender, signalSender, reciever, signalReciever);
    }

    ~IgnoreSignal()
    {
        QObject::connect(sender, signalSender, reciever, signalReciever, Qt::UniqueConnection);
    }

    Sender sender;
    SignalSender signalSender;
    Reciever reciever;
    SignalReciever signalReciever;
};

template <typename Sender, typename SignalSender,
          typename Reciever, typename SignalReciever>
auto ignoreSignal(Sender sender, SignalSender signalSender,
                  Reciever reciever, SignalReciever signalReciever)
{
    return IgnoreSignal<Sender, SignalSender, Reciever, SignalReciever>{
        sender, signalSender, reciever, signalReciever
    };
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

    auto& ig = parent;

    connect(&ig, &GtIntelliGraph::nodeAppended,
            this, &GtIntelliGraphModelAdapter::appendNodeToModel, Qt::UniqueConnection);
    connect(&ig, &GtIntelliGraph::connectionAppended,
            this, &GtIntelliGraphModelAdapter::appendConnectionToModel, Qt::UniqueConnection);
    connect(&ig, &GtIntelliGraph::nodePositionChanged,
            this, [m = m_graphModel.get()](gt::ig::NodeId nodeId, QPointF pos){
        m->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    // setup all other subgraphs
    for (auto* graph : ig.subGraphs())
    {
        graph->initGroupProviders();
        graph->makeModelAdapter(gt::ig::DummyModel);
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
            this, &GtIntelliGraphModelAdapter::appendNodeFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, &GtIntelliGraphModelAdapter::onNodeDeletedFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
            this, &GtIntelliGraphModelAdapter::appendConnectionFromModel, Qt::UniqueConnection);
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, &GtIntelliGraphModelAdapter::onConnectionDeletedFromModel, Qt::UniqueConnection);
}

GtIntelliGraphModelAdapter::~GtIntelliGraphModelAdapter()
{
    for (auto* node : intelliGraph().nodes())
    {
        node->setExecutor(gt::ig::NoExecutor);
    }
}

GtIntelliGraph&
GtIntelliGraphModelAdapter::intelliGraph()
{
    auto tmp = qobject_cast<GtIntelliGraph*>(parent());
    assert(tmp);
    return *tmp;
}

const GtIntelliGraph&
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
    // dont close an active model if we are not forcing it
    return m_policy == gt::ig::DummyModel || force;
}

void
GtIntelliGraphModelAdapter::mergeConnections(GtIntelliGraph& ig)
{
    gtTrace().verbose() << __FUNCTION__ << ig.objectName();

    auto const& connections = ig.connections();

    for (auto* con : connections)
    {
        if (m_graphModel->nodeExists(con->outNodeId()) &&
            m_graphModel->nodeExists(con->inNodeId()) &&
            !m_graphModel->connectionExists(con->connectionId()))
        {
            appendConnectionToModel(con);
        }
    }
}

void
GtIntelliGraphModelAdapter::mergeGraphModel(GtIntelliGraph& ig)
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
        std::vector<GtIntelliGraphConnection*> nodeConnections;
        std::copy_if(std::cbegin(connections), std::cend(connections),
                     std::back_inserter(nodeConnections),
                     [id = node->id(), m = m_graphModel.get()](auto const* con){
            return m->nodeExists(con->inNodeId()) && m->nodeExists(con->outNodeId()) &&
                   (con->inNodeId()  == id || con->outNodeId() == id);
        });

        for (auto* con : qAsConst(nodeConnections))
        {
            auto conId = con->connectionId();
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
GtIntelliGraphModelAdapter::appendNodeFromModel(QtNodes::NodeId nodeId)
{
    auto& ig = intelliGraph();

    auto* model = m_graphModel->delegateModel<GtIntelliGraphObjectModel>(nodeId);
    if (!model)
    {
        gtWarning() << tr("Unkown delegate model! (id: %1)").arg(nodeId);
        return false;
    }

    // move node from model to object tree
    auto node = std::unique_ptr<GtIntelliGraphNode>(model->node());
    if (!node)
    {
        gtError() << tr("Invalid delegate model! (id: %1)").arg(nodeId);
        return false;
    }

    node->setId(gt::ig::NodeId::fromValue(nodeId));

    auto cmd = gtApp->startCommand(&ig, tr("Appending node '%1'").arg(node->objectName()));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    auto ignore = ignoreSignal(
        &ig, &GtIntelliGraph::nodeAppended,
        this, &GtIntelliGraphModelAdapter::appendNodeToModel
    );

    if (auto* n = ig.appendNode(std::move(node), gt::ig::KeepNodeId))
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
GtIntelliGraphModelAdapter::appendConnectionFromModel(QtNodes::ConnectionId conId)
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
        &ig, &GtIntelliGraph::connectionAppended,
        this, &GtIntelliGraphModelAdapter::appendConnectionToModel
    );

    if (auto* con = ig.appendConnection(std::make_unique<GtIntelliGraphConnection>(conId)))
    {
        setupConnection(*con);
        return true;
    }

    gtError() << tr("Failed to append connection to graph model!")
              << conId;
    return false;
}

bool
GtIntelliGraphModelAdapter::appendNodeToModel(GtIntelliGraphNode* node)
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
    auto model = std::make_unique<GtIntelliGraphObjectModel>(*node);

    if (m_graphModel->addNode(std::move(model), nodeId) == QtNodes::InvalidNodeId)
    {
        gtError() << tr("Failed to add node %1 to the graph model! Error:")
                         .arg(node->modelName());
        return false;
    }

    {
        auto ignore = ignoreSignal(
            m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
            this, &GtIntelliGraphModelAdapter::appendNodeFromModel
        );

        emit m_graphModel->nodeCreated(nodeId);
    }

    // update node position
    ig.setNodePosition(node, node->pos());

    setupNode(*node);

    return true;
}

bool
GtIntelliGraphModelAdapter::appendConnectionToModel(GtIntelliGraphConnection* connection)
{
    if (!connection) return false;

    auto conId = connection->connectionId();

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
            this, &GtIntelliGraphModelAdapter::appendConnectionFromModel
        );

        m_graphModel->addConnection(conId);
    }

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
                if (con->connectionId() == conId) cons.removeOne(con);
            }
        }
    }

    qDeleteAll(cons);
    qDeleteAll(nodes);
}

void
GtIntelliGraphModelAdapter::setupNode(GtIntelliGraphNode& node)
{
    connect(&node, &QObject::destroyed, m_graphModel.get(),
            [model = m_graphModel.get(),
             name = node.modelName(),
             nodeId = node.id(),
             this](){
        auto ignore = ignoreSignal(
            model, &QtNodes::DataFlowGraphModel::nodeDeleted,
            this, &GtIntelliGraphModelAdapter::onNodeDeletedFromModel
        );
        gtDebug().verbose() << "Deleting node from model:" << name
                            << "(" << nodeId << ")";;
        model->deleteNode(nodeId);
    });

    connect(&node, &GtIntelliGraphNode::nodeChanged, m_graphModel.get(),
            [model = m_graphModel.get(),
             nodeId = node.id()](){
        emit model->nodeUpdated(nodeId);
    });

    auto updateNodeFlags = [model = m_graphModel.get(),
                            nodeId = node.id()](){
        emit model->nodeFlagsUpdated(nodeId);
    };

    connect(&node, &GtIntelliGraphNode::computingStarted,
            m_graphModel.get(), updateNodeFlags);
    connect(&node, &GtIntelliGraphNode::computingFinished,
            m_graphModel.get(), updateNodeFlags);

    // init input output providers
    if (auto group = qobject_cast<GtIntelliGraph*>(&node))
    {
        group->initGroupProviders();
        group->makeModelAdapter(gt::ig::DummyModel);
    }

    node.setExecutor(gt::ig::DefaultExecutor);
}

void
GtIntelliGraphModelAdapter::setupConnection(GtIntelliGraphConnection& connection)
{
    connect(&connection, &QObject::destroyed,m_graphModel.get(),
            [model = m_graphModel.get(),
             conId = connection.connectionId(),
             this](){
        auto ignore = ignoreSignal(
            model, &QtNodes::DataFlowGraphModel::connectionDeleted,
            this, &GtIntelliGraphModelAdapter::onConnectionDeletedFromModel
        );
        gtDebug().verbose() << "Deleting connection from model:" << conId;
        model->deleteConnection(conId);
    });
}

void
GtIntelliGraphModelAdapter::onNodeDeletedFromModel(QtNodes::NodeId nodeId)
{
    auto& ig = intelliGraph();

    auto cmd = gtApp->startCommand(&ig, tr("Deleting node '%1'").arg(nodeId));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    ig.deleteNode(gt::ig::NodeId::fromValue(nodeId));
}

void
GtIntelliGraphModelAdapter::onConnectionDeletedFromModel(QtNodes::ConnectionId conId)
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

    ig.deleteConnection(conId);
}

