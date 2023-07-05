/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_coredatamodel.h"

#include <QJsonObject>
#include <QJsonArray>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModel>

GTIG_REGISTER_NODE(GtIntelliGraph, "Group")

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
        return conId == connection->toConnectionId();
    });

    return iter == std::end(connections) ? nullptr : *iter;
}

GtIntelliGraph::GtIntelliGraph() :
    GtIntelliGraphNode("Intelli Graph")
{

}

void
GtIntelliGraph::initInputOutputProvider()
{
#if 1
    auto* exstInput = findDirectChild<GtIgGroupInputProvider*>();
    auto input = exstInput ? nullptr : std::make_unique<GtIgGroupInputProvider>();

    auto* exstOutput = findDirectChild<GtIgGroupOutputProvider*>();
    auto output = exstOutput ? nullptr : std::make_unique<GtIgGroupOutputProvider>();
#else
    auto* exstInput = findDirectChild<GtIgGroupInputProvider*>();
    auto input = exstInput ? std::unique_ptr<GtIgGroupInputProvider>(exstInput) :
                             std::make_unique<GtIgGroupInputProvider>();
    input->setParent(nullptr);
    input->disconnect(this);

    auto* exstOutput = findDirectChild<GtIgGroupOutputProvider*>();
    auto output = exstOutput ? std::unique_ptr<GtIgGroupOutputProvider>(exstOutput) :
                               std::make_unique<GtIgGroupOutputProvider>();
    output->setParent(nullptr);
    input->disconnect(this);

    auto onPortInserted = [this](auto* provider, PortType type, PortIndex idx){
        PortId id = provider->portId(provider->INVERSE_TYPE(), idx);
        if (auto* port = provider->port(id))
        {
            provider->TYPE() == PortType::In ?
                insertInPort(*port, idx) :
                insertOutPort(*port, idx);
            return true;
        }
        return false;
    };

    auto onPortChanged = [this](auto* provider, PortId id){
        auto* inPort = provider->port(id);
        auto  idx    = provider->portIndex(provider->INVERSE_TYPE(), id);
        auto* port   = this->port(portId(provider->TYPE(), idx));

        if (!inPort || !port) return;

        port->typeId = inPort->typeId;
        port->caption = inPort->caption;
        emit portChanged(port->id());
    };

    auto onPortDeleted = [this](auto* provider, PortIndex idx){
        removePort(portId(provider->INVERSE_TYPE(), idx));
    };

    connect(input.get(), &GtIntelliGraphNode::portInserted,
            this, [=, in = input.get()](PortType type, PortIndex idx){
                onPortInserted(in, type, idx);
            });
    connect(input.get(), &GtIntelliGraphNode::portChanged,
            this, [=, in = input.get()](PortId id){
                onPortChanged(in, id);
            });
    connect(input.get(), &GtIntelliGraphNode::portAboutToBeDeleted,
            this, [=, in = input.get()](PortType, PortIndex idx){
                onPortDeleted(in, idx);
            });

    connect(output.get(), &GtIntelliGraphNode::portInserted,
            this, [=, out = output.get()](PortType type, PortIndex idx){
                if (onPortInserted(out, type, idx))
                {
                    m_outData.insert(std::next(m_outData.begin(), idx), NodeData{});
                }
            });
    connect(output.get(), &GtIntelliGraphNode::portChanged,
            this, [=, out = output.get()](PortId id){
                onPortChanged(out, id);
            });
    connect(output.get(), &GtIntelliGraphNode::portAboutToBeDeleted,
            this, [=, out = output.get()](PortType, PortIndex idx){
                onPortDeleted(out, idx);
            });

    connect(output.get(), &GtIntelliGraphNode::outDataUpdated,
            this, &GtIntelliGraphNode::outDataUpdated);
    connect(output.get(), &GtIntelliGraphNode::outDataInvalidated,
            this, &GtIntelliGraphNode::outDataInvalidated);
#endif

    appendNode(std::move(output));
    appendNode(std::move(input));
}

void
GtIntelliGraph::setNewNodeId(GtIntelliGraphNode& node)
{
    auto const nodes = findDirectChildren<GtIntelliGraphNode const*>();

    // id may already be used
    QVector<double> ids;
    ids.reserve(nodes.size());
    std::transform(std::begin(nodes), std::end(nodes), std::back_inserter(ids),
                   [](GtIntelliGraphNode const* n){ return n->id(); });

    if (ids.contains(node.id()))
    {
        // generate a new one
        auto maxId = *std::max_element(std::begin(ids), std::end(ids)) + 1;
        node.setId(gt::ig::NodeId::fromValue(maxId));
        assert(node.id() != gt::ig::invalid<NodeId>());
    }
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
    return findDirectChildren<GtIntelliGraphConnection*>();
}

QList<GtIntelliGraphConnection const*>
GtIntelliGraph::connections() const
{
    return gt::container_const_cast(
        const_cast<GtIntelliGraph*>(this)->connections()
    );
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
GtIntelliGraph::findNode(QtNodeId nodeId)
{
    auto const nodes = findDirectChildren<GtIntelliGraphNode*>();

    return ::findNode(nodes, nodeId);
}

GtIntelliGraphNode const*
GtIntelliGraph::findNode(QtNodeId nodeId) const
{
    return const_cast<GtIntelliGraph*>(this)->findNode(nodeId);
}

GtIntelliGraphConnection*
GtIntelliGraph::findConnection(QtConnectionId const& conId)
{
    auto const connections = findDirectChildren<GtIntelliGraphConnection*>();

    return ::findConnection(connections, conId);
}

GtIntelliGraphConnection const*
GtIntelliGraph::findConnection(QtConnectionId const& conId) const
{
    return const_cast<GtIntelliGraph*>(this)->findConnection(conId);
}

void
GtIntelliGraph::insertOutData(PortIndex idx)
{
    m_outData.insert(std::next(m_outData.begin(), idx), NodeData{});
}

bool
GtIntelliGraph::setOutData(PortIndex idx, NodeData data)
{
    if (idx >= m_outData.size())
    {
        gtError().medium() << tr("Failed to set out data! (Index out of bounds)");
        return false;
    }

    gtDebug().verbose() << "Setting group output data:" << data;

    m_outData.at(idx) = std::move(data);

    updatePort(idx);

    return true;
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

    PortIndex idx{0};

    // this will trigger the evaluation
    in->updateNode();

    // idealy now the data should have been set
    if (m_outData.size() != out->ports(PortType::In).size())
    {
        gtWarning().medium()
            << tr("Group out data mismatches output provider! (%1 vs %2)")
                   .arg(out->ports(PortType::In).size())
                   .arg(m_outData.size());
        return {};
    }

    idx = portIndex(PortType::Out, outId);

    return m_outData.at(idx);
}

void
GtIntelliGraph::clear()
{
    auto cons = findDirectChildren<GtIntelliGraphConnection*>();
    auto nodes = findDirectChildren<GtIntelliGraphNode*>();

    GtObjectList objects;
    objects.reserve(cons.size() + nodes.size());
    std::copy(std::cbegin(cons), std::cend(cons), std::back_inserter(objects));
    std::copy(std::cbegin(nodes), std::cend(nodes), std::back_inserter(objects));

    gtDataModel->deleteFromModel(objects);
}

void
GtIntelliGraph::removeOrphans(DataFlowGraphModel& model)
{
    auto nodes = this->nodes();
    auto cons = this->connections();

    for (auto nodeId : model.allNodeIds())
    {
        foreach (auto* node, qAsConst(nodes))
        {
            if (auto* m = model.delegateModel<QtNodes::NodeDelegateModel>(nodeId))
            {
                if (node->id() == nodeId && m->name() == node->modelName())
                {
                    nodes.removeOne(node);
                }
            }
        }

        for (auto conId : model.allConnectionIds(nodeId))
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

    gtDataModel->deleteFromModel(objects);
}

QJsonObject
GtIntelliGraph::toJson(bool clone) const
{
    QJsonObject json;

    QJsonArray connections;
    for (auto* connection : findDirectChildren<GtIntelliGraphConnection*>())
    {
        connections.append(connection->toJson());
    }
    json["connections"] = connections;

    QJsonArray nodes;
    for (auto* node : findDirectChildren<GtIntelliGraphNode*>())
    {
        nodes.append(node->toJson(clone));
    }
    json["nodes"] = nodes;

    return json;
}

bool
GtIntelliGraph::fromJson(const QJsonObject& json)
{
    // for now we clear the whole object tree -> we may optimize later
    clear();

    bool success = true;

    auto const connections = json["connections"].toArray();

    for (auto const& connection : connections)
    {
        auto* obj = new GtIntelliGraphConnection(this);
        if (!obj->fromJson(connection.toObject()))
        {
            gtWarning() << tr("Failed to restore connection:") << obj->objectName();
            success = false; break;
        }
        gtDebug() << "Restored connection:" << obj->objectName();
    }

    auto const nodes = json["nodes"].toArray();

    try
    {
        for (auto const& node : nodes)
        {
            auto obj = GtIntelliGraphNode::fromJson(node.toObject());
            if (!obj->isValid())
            {
                gtWarning() << tr("Failed to restore node:") << obj->objectName();
                success = false; break;
            }
            gtDebug() << "Restored node:" << obj->objectName();
            obj.release()->setParent(this);
        }
    }
    catch (std::exception const& e)
    {
        gtError() << tr("Failed to restore Intelli Graph from json! Error:")
                  << e.what();
        success = false;
    }

    return success;
}

GtIntelliGraphNode*
GtIntelliGraph::createNode(QtNodeId nodeId)
{
    if (!m_activeGraphModel) return nullptr;

    auto* model = m_activeGraphModel->delegateModel<GtIntelliGraphObjectModel>(nodeId);

    if (!model) return nullptr;

    GtIntelliGraphNode* node = findNode(nodeId);

    // update node in model
    if (node && node->isValid(model->name()))
    {
        gtWarning().verbose()
            << tr("Node '%1' already exists!").arg(nodeId);

        model->init(*node);
    }
    // move node from model to object tree
    else
    {
        // node is not compatible
        if (node) node->deleteLater();

        node = &model->node();

        assert (node->parent() == model);

        node->setId(gt::ig::NodeId{nodeId});

        gtDebug().verbose() << "Creating node:" << node;

        node->setParent(nullptr); // to avoid disconnect-error msg (see #520)

        if (!gtDataModel->appendChild(node, this).isValid())
        {
            gtError() << tr("Failed to append node:") << node;
            node->deleteLater();
            return nullptr;
        }
    }

    node->updateObjectName();
    node->updateNode();

    setupNode(*node);

    updateNodePosition(nodeId);

    return node;
}

bool
GtIntelliGraph::appendNode(std::unique_ptr<GtIntelliGraphNode> node)
{
    if (!node) return false;

    setNewNodeId(*node);

    if (!gtDataModel->appendChild(node.get(), this).isValid()) return false;

    // update graph model
    if (m_activeGraphModel) appendNodeToModel(*node);

    node.release();
    return true;
}

bool
GtIntelliGraph::deleteNode(QtNodeId nodeId)
{
    if (auto* node = findNode(nodeId))
    {
        gtDebug().verbose()
            << "Deleting node:" << node;
        return gtDataModel->deleteFromModel(node);
    }
    return false;
}

GtIntelliGraphConnection*
GtIntelliGraph::createConnection(const QtConnectionId& connectionId)
{
    if (auto* connection = findConnection(connectionId))
    {
        gtWarning().verbose() << tr("Connection was already created!") << connectionId;

        setupConnection(*connection);

        return nullptr;
    }

    gtDebug().verbose()
        << "Creating connection:" << connectionId;

    auto connection = std::make_unique<GtIntelliGraphConnection>();
    connection->fromConnectionId(connectionId);

    if (!gtDataModel->appendChild(connection.get(), this).isValid())
    {
        return nullptr;
    }

    setupConnection(*connection);

    return connection.release();
}

bool
GtIntelliGraph::appendConnection(std::unique_ptr<GtIntelliGraphConnection> connection)
{
    // connection may already exist
    if (findConnection(connection->toConnectionId())) return false;

    if (!gtDataModel->appendChild(connection.get(), this).isValid()) return false;

    // update graph model
    if (m_activeGraphModel) appendConnectionToModel(*connection);

    connection.release();
    return true;
}

bool
GtIntelliGraph::deleteConnection(const QtConnectionId& connectionId)
{
    if (auto* connection = findConnection(connectionId))
    {
        gtDebug().verbose() << "Deleting connection:" << connectionId;
        return gtDataModel->deleteFromModel(connection);
    }
    return false;
}

bool
GtIntelliGraph::updateNodePosition(QtNodeId nodeId)
{
    if (!m_activeGraphModel) return false;

    auto* delegate = m_activeGraphModel->delegateModel<GtIntelliGraphObjectModel>(nodeId);

    if (!delegate) return false;

    auto position = m_activeGraphModel->nodeData(nodeId, QtNodes::NodeRole::Position);

    if (!position.isValid()) return false;

    auto pos = position.toPointF();

    auto& node = delegate->node();

    gtDebug().verbose()
        << "Updating node position to" << pos << gt::brackets(node.objectName());

    node.setPos(pos);

    return true;
}

void
GtIntelliGraph::setActiveGraphModel(DataFlowGraphModel& model)
{
    m_activeGraphModel = &model;
}

GtIntelliGraph::DataFlowGraphModel*
GtIntelliGraph::activeGraphModel()
{
    return m_activeGraphModel;
}

GtIntelliGraph::DataFlowGraphModel const*
GtIntelliGraph::activeGraphModel() const
{
    return m_activeGraphModel;
}

void
GtIntelliGraph::clearActiveGraphModel()
{
    m_activeGraphModel.clear();
}

void
GtIntelliGraph::onObjectDataMerged()
{
    if (!m_activeGraphModel) return;

    // after undo/redo we may have to add resoted nodes and connections to
    // the model
    auto const& nodes = this->nodes();
    auto const& connections = this->connections();

    auto const& modelNodes = m_activeGraphModel->allNodeIds();

    for (auto* node : nodes)
    {
        if (modelNodes.find(node->id()) == modelNodes.end())
        {
            gtDebug().verbose().nospace()
                << "### Merging node " << node->modelName()
                << "(" << node->id() << ")";

            // update graph model
            appendNodeToModel(*node);
        }

        auto const& modelConnections = m_activeGraphModel->allConnectionIds(node->id());

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
                appendConnectionToModel(*con);
            }
        }
    }
}

void
GtIntelliGraph::setupNode(GtIntelliGraphNode& node)
{
    connect(&node, &QObject::destroyed, m_activeGraphModel,
            [graph = m_activeGraphModel.data(),
             model = node.modelName(),
             nodeId = node.id()](){
        gtDebug().verbose() << "Deleting node from model:" << model
                            << "(" << nodeId << ")";;
        graph->deleteNode(nodeId);
    });
    connect(&node, &GtIntelliGraphNode::nodeChanged, m_activeGraphModel,
            [graph = m_activeGraphModel.data(),
             nodeId = node.id()](){
        emit graph->nodeUpdated(nodeId);
    });

    // init input output providers
    if (auto group = qobject_cast<GtIntelliGraph*>(&node))
    {
        group->initInputOutputProvider();
    }
}

void
GtIntelliGraph::setupConnection(GtIntelliGraphConnection& connection)
{
    connect(&connection, &QObject::destroyed,m_activeGraphModel,
            [graph = m_activeGraphModel.data(),
             conId = connection.toConnectionId()](){
        gtDebug().verbose() << "Deleting connection from model:" << conId;
        graph->deleteConnection(conId);
    });
}

bool
GtIntelliGraph::appendNodeToModel(GtIntelliGraphNode& node)
{
    assert(m_activeGraphModel);

    try
    {
        m_activeGraphModel->loadNode(node.toJson());
    }
    catch (std::exception const& e)
    {
        gtError() << tr("Failed to add node %1 to graph model! Error:")
                     .arg(node.modelName())
                  << gt::quoted(std::string{e.what()});
        return false;
    }

    setupNode(node);
    return true;
}

bool
GtIntelliGraph::appendConnectionToModel(GtIntelliGraphConnection& con)
{
    assert(m_activeGraphModel);

    m_activeGraphModel->addConnection(con.toConnectionId());
    setupConnection(con);
    return true;
}
