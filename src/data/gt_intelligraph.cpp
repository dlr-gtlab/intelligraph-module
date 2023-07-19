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

#include "gt_coredatamodel.h"

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
        return conId == connection->toConnectionId();
    });

    return iter == std::end(connections) ? nullptr : *iter;
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
GtIntelliGraph::findNode(QtNodeId nodeId)
{
    return ::findNode(nodes(), nodeId);
}

GtIntelliGraphNode const*
GtIntelliGraph::findNode(QtNodeId nodeId) const
{
    return const_cast<GtIntelliGraph*>(this)->findNode(nodeId);
}

GtIntelliGraphConnection*
GtIntelliGraph::findConnection(QtConnectionId const& conId)
{
    return ::findConnection(connections(), conId);
}

GtIntelliGraphConnection const*
GtIntelliGraph::findConnection(QtConnectionId const& conId) const
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

GtIntelliGraphNode*
GtIntelliGraph::appendNode(std::unique_ptr<GtIntelliGraphNode> node)
{
    if (!node) return {};

    updateNodeId(*node);

    if (!gtDataModel->appendChild(node.get(), this).isValid()) return {};

    // update graph model
    emit nodeAppended(node.get());

    return node.release();
}

GtIntelliGraphConnection*
GtIntelliGraph::appendConnection(std::unique_ptr<GtIntelliGraphConnection> connection)
{
    // connection may already exist
    if (!connection || findConnection(connection->toConnectionId())) return {};

    if (!gtDataModel->appendChild(connection.get(), &connectionGroup()).isValid()) return {};

    // update graph model
    emit connectionAppended(connection.get());

    return connection.release();
}

bool
GtIntelliGraph::deleteNode(QtNodeId nodeId)
{
    if (auto* node = findNode(nodeId))
    {
        gtInfo().verbose()
            << tr("Deleting node:") << node->objectName();
        return gtDataModel->deleteFromModel(node);
    }
    return false;
}

bool
GtIntelliGraph::deleteConnection(const QtConnectionId& connectionId)
{
    if (auto* connection = findConnection(connectionId))
    {
        gtInfo().verbose()
            << tr("Deleting connection:") << connection->objectName();
        return gtDataModel->deleteFromModel(connection);
    }
    return false;
}

void
GtIntelliGraph::setNodePosition(QtNodeId nodeId, QPointF pos)
{
    if (auto* node = findNode(nodeId))
    {
        node->setPos(pos);
        emit nodePositionChanged(NodeId::fromValue(nodeId), pos);
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
        gtWarning() << QObject::tr("Model manager not found!");
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

void
GtIntelliGraph::updateNodeId(GtIntelliGraphNode& node)
{
    auto const nodes = findDirectChildren<GtIntelliGraphNode const*>();

    // id may already be used
    QVector<NodeId> ids;
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
