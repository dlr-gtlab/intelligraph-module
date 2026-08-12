/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/graphdatamodel.h"
#include "intelli/graph.h"
#include "intelli/graphuservariables.h"
#include "intelli/private/utils.h"

#include <gt_coreapplication.h>
#include <gt_project.h>

#include <QPointer>

using namespace intelli;

struct PortDataItem
{
    /// referenced port
    PortId portId;
    /// actual data at port
    NodeDataSet data{nullptr};
};

struct NodeDataItem
{
    /// only the output data is stored
    QVector<PortDataItem> outputPortData;
    /// internal evalution state
    NodeEvalState state = NodeEvalState::Outdated;
};

struct GraphDataModel::Impl
{
    Impl(Graph& g) : graph(&g) {}

    /// referencing graph
    QPointer<Graph> graph;
    /// scope object
    QPointer<GtObject> scope = {};
    /// data map
    QHash<NodeUuid, NodeDataItem> data = {};
    /// logging indicator
    bool silent = false;

    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const
    {
        auto nodeEntry = data.constFind(nodeUuid);
        if (nodeEntry == data.constEnd())
        {
            if (!silent)
                gtError() << tr("failed to find data for node '%1'! (entry not found)")
                                 .arg(nodeUuid);
            return {};
        }

        auto portEntry = std::find_if(nodeEntry->outputPortData.cbegin(),
                                      nodeEntry->outputPortData.cend(),
                                      [portId](PortDataItem const& item){
            return item.portId == portId;
        });
        if (portEntry == nodeEntry->outputPortData.cend())
        {
            if (!silent)
                gtError() << tr("failed to find data for port '%2' of node '%1'!")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return {};
        }

        if (!silent)
            gtTrace().verbose()
                << tr("Accessing node data of '%1', port: %3, data: %2")
                       .arg(nodeUuid, toString(portEntry->data.ptr), toString(portId));

        return portEntry->data;
    }

    bool setNodeData(GraphDataModel& model, NodeUuid const& nodeUuid, PortId portId, NodeDataSet dset)
    {
        auto nodeEntry = data.find(nodeUuid);
        if (nodeEntry == data.end())
        {
            if (!silent)
                gtError() << tr("failed to set data for node '%1'! (entry not found)")
                                 .arg(nodeUuid);
            return false;
        }

        auto portEntry = std::find_if(nodeEntry->outputPortData.begin(), nodeEntry->outputPortData.end(),
                                      [portId](PortDataItem const& item){
            return item.portId == portId;
        });
        if (portEntry == nodeEntry->outputPortData.end())
        {
            if (!silent)
                gtError() << tr("failed to set data for port '%2' of node '%1'! "
                                "(out port not found)")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return false;
        }

        if (!silent)
            gtTrace().verbose()
                << tr("Setting node data for '%1', port: %3, data: %2")
                       .arg(nodeUuid, toString(dset.ptr), toString(portId));

        portEntry->data = std::move(dset);

        // TODO: need to update successors?
        Impl::propagate(model, nodeUuid, &GraphDataModel::setNodeEvaluationOutdated);
        return true;
    }

    static void propagate(GraphDataModel& model,
                          NodeUuid const& source,
                          void (GraphDataModel::*functor)(NodeUuid const&))
    {
        for (auto& successor : model.pimpl->graph->globalConnectionModel()
                                   .iterateNodes(source, PortType::Out))
        {
            (model.*functor)(successor);
        }
    }
};

GraphDataModel::GraphDataModel(Graph& graph) :
    pimpl(std::make_unique<Impl>(graph))
{
    setParent(&graph);

    setObjectName(QStringLiteral("__data_model"));

    if (gtApp) pimpl->scope = gtApp->currentProject();

    reset();
}

GraphDataModel::~GraphDataModel() = default;

bool
GraphDataModel::isSilent() const
{
    return pimpl->silent;
}

void
GraphDataModel::setSilent(bool value)
{
    pimpl->silent = value;
}

void
GraphDataModel::reset()
{
    if (!isSilent())
        gtTrace().verbose()
            << tr("Resetting the data model of '%1'")
                   .arg(relativeNodePath(graph()));

    pimpl->data.clear();

    Graph& graph = this->graph();
    setupConnections(graph);

    auto const& nodes = graph.nodes();
    for (Node* node : nodes)
    {
        onNodeAppended(node);
    }
}

Graph&
GraphDataModel::graph()
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

Graph const&
GraphDataModel::graph() const
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

NodeDataSet
GraphDataModel::nodeData(Graph const& graph,
                         NodeId nodeId,
                         PortId portId) const
{
    auto& conModel = graph.connectionModel();
    auto entry = conModel.find(nodeId);
    if (entry == conModel.end())
    {
        if (!isSilent())
            gtError() << tr("failed to access data, node '%1' not found in graph '%2'!")
                             .arg(nodeId).arg(relativeNodePath(graph));
        return {};
    }

    Node const* node = entry->node;
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to access data, invalid node '%1' in graph '%2'!")
                             .arg(nodeId).arg(relativeNodePath(graph));
        return {};
    }

    if (node->portType(portId) == PortType::In)
    {
        for (auto& connection : entry->iterate())
        {
            if (connection.sourcePort != portId) continue;

            node = conModel.node(connection.node);
            if (!node) break;

            return pimpl->nodeData(node->uuid(), connection.port);
        }

        if (!isSilent())
            gtTrace().verbose()
                << tr("failed to access data for node '%1' in graph '%2', "
                      "port %3 not found! (not connected?)")
                       .arg(nodeId)
                       .arg(relativeNodePath(graph))
                       .arg(portId);

        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataSet
GraphDataModel::nodeData(NodeUuid const& nodeUuid,
                         PortId portId) const
{
    auto& conModel = graph().globalConnectionModel();
    auto entry = conModel.find(nodeUuid);
    if (entry == conModel.end())
    {
        if (!isSilent())
            gtError() << tr("failed to access data, node '%1' not found in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    Node const* node = entry->node;
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to access data, invalid node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    if (node->portType(portId) == PortType::In)
    {
        for (auto& connection : entry->iterate())
        {
            if (connection.sourcePort != portId) continue;

            return pimpl->nodeData(connection.node, connection.port);
        }

        if (!isSilent())
            gtTrace().verbose()
                << tr("failed to access data for node '%1' in graph '%2', "
                      "port %3 not found! (not connected?)")
                       .arg(nodeUuid, relativeNodePath(graph()))
                       .arg(portId);

        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataSet
GraphDataModel::nodeData(NodeUuid const& nodeUuid,
                         PortType type,
                         PortIndex portIdx) const
{
    auto& conModel = graph().globalConnectionModel();
    auto entry = conModel.find(nodeUuid);
    if (entry == conModel.end())
    {
        if (!isSilent())
            gtError() << tr("failed to access data, node '%1' not found in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    Node const* node = entry->node;
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to access data, invalid node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    PortId portId = node->portId(type, portIdx);
    if (!portId.isValid())
    {
        if (!isSilent())
            gtError() << tr("failed to access data for node '%1': invalid port %2 (%3)!")
                             .arg(nodeUuid)
                             .arg(portIdx)
                             .arg(toString(type));
        return {};
    }

    if (type == PortType::In)
    {
        for (auto& connection : entry->iterate())
        {
            if (connection.sourcePort != portId) continue;

            return pimpl->nodeData(connection.node, connection.port);
        }

        if (!isSilent())
            gtTrace().verbose()
                << tr("failed to access data for node '%1', "
                      "port %2 (%3) not found! (not connected?)")
                       .arg(nodeUuid)
                       .arg(portIdx)
                       .arg(toString(type));
        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataPtrList
GraphDataModel::nodeData(NodeUuid const& nodeUuid,
                         PortType type) const
{
//    assert(!"function should not be needed");
//    assert(type == PortType::Out);
//    if (type != PortType::Out) return {};

    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to find data for node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    auto const& ports = node->ports(type);

    NodeDataPtrList list;
    list.reserve(ports.size());
    for (auto const& port : ports)
    {
        list.push_back({ port.id(), nodeData(nodeUuid, port.id()) });
    }

    return list;
}

bool
GraphDataModel::setNodeData(Graph const& graph,
                            NodeId nodeId,
                            PortId portId,
                            NodeDataSet data)
{
    auto& conModel = graph.connectionModel();
    auto entry = conModel.find(nodeId);
    if (entry == conModel.end())
    {
        if (!isSilent())
            gtError() << tr("failed to set data, node '%1' not found in graph '%2'!")
                             .arg(nodeId).arg(relativeNodePath(graph));
        return false;
    }

    Node const* node = entry->node;
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to set data, invalid node '%1' in graph '%2'!")
                             .arg(nodeId).arg(relativeNodePath(graph));
        return false;
    }

    if (node->portType(portId) != PortType::Out)
    {
        if (!isSilent())
            gtError() << tr("failed to set data of node '%1' in graph '%2', "
                            "not an output port type!")
                             .arg(nodeId).arg(relativeNodePath(graph));
        return false;
    }

    return pimpl->setNodeData(*this, node->uuid(), portId, std::move(data));
}

bool
GraphDataModel::setNodeData(NodeUuid const& nodeUuid,
                            PortId portId,
                            NodeDataSet data)
{
    return pimpl->setNodeData(*this, nodeUuid, portId, std::move(data));
}

bool
GraphDataModel::setNodeData(NodeUuid const& nodeUuid,
                            PortType type,
                            PortIndex portIdx,
                            NodeDataSet data)
{
    auto& conModel = graph().globalConnectionModel();
    auto entry = conModel.find(nodeUuid);
    if (entry == conModel.end())
    {
        if (!isSilent())
            gtError() << tr("failed to set data, node '%1' not found in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return false;
    }

    Node const* node = entry->node;
    if (!node)
    {
        if (!isSilent())
            gtError() << tr("failed to set data, invalid node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return false;
    }

    PortId portId = node->portId(type, portIdx);
    if (!portId.isValid())
    {
        if (!isSilent())
            gtError() << tr("failed to set data for node '%1': invalid port %2 (%3)!")
                             .arg(nodeUuid)
                             .arg(portIdx)
                             .arg(toString(type));
        return false;
    }

    if (type != PortType::Out)
    {
        if (!isSilent())
            gtError() << tr("failed to set data of node '%1' in graph '%2', "
                            "not an output port type!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return false;
    }

    return pimpl->setNodeData(*this, node->uuid(), portId, std::move(data));
}

bool
GraphDataModel::setNodeData(NodeUuid const& nodeUuid,
                            PortType type,
                            NodeDataPtrList const& data)
{
//    assert(!"function should not be needed");
    if (type != PortType::Out) return true;

    for (auto const& entry : data)
    {
        PortId portId = entry.first;
        if (!pimpl->setNodeData(*this, nodeUuid, portId, std::move(entry.second)))
        {
            if (!isSilent())
                gtError() << tr("failed to set data for port '%2' of node '%1'!")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return false;
        }
    }

    return true;
}

NodeEvalState
GraphDataModel::nodeEvalState(NodeUuid const& nodeUuid) const
{
    auto nodeEntry = pimpl->data.constFind(nodeUuid);
    if (nodeEntry == pimpl->data.constEnd())
    {
        if (!isSilent())
            gtError()
                << tr("failed to find data for node '%1'!")
                       .arg(nodeUuid);
        return {};
    }

    // TODO
    return nodeEntry->state;
}

void
GraphDataModel::nodeEvaluationStarted(NodeUuid const& nodeUuid)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent())
            gtError()
                << tr("failed to find data for node '%1'!")
                       .arg(nodeUuid);
        return;
    }

    // TODO
    if (nodeEntry->state == NodeEvalState::Outdated)
    {
        nodeEntry->state = NodeEvalState::Evaluating;
    }

    emit evaluationStarted(nodeUuid);
}

void
GraphDataModel::nodeEvaluationFinished(NodeUuid const& nodeUuid)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent())
            gtError()
                << tr("failed to find data for node '%1'!")
                       .arg(nodeUuid);
        return;
    }

    // TODO
    if (nodeEntry->state == NodeEvalState::Evaluating)
    {
        nodeEntry->state = NodeEvalState::Valid;
    }

    emit evaluationFinished(nodeUuid);
}

void
GraphDataModel::setNodeEvaluationFailed(NodeUuid const& nodeUuid)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent())
            gtError() << tr("failed to invalidate node '%1'!")
                             .arg(nodeUuid);
        return;
    }

    if (nodeEntry->state == NodeEvalState::Invalid)
    {
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Marking the data of the node '%1' as invalid")
                   .arg(nodeUuid);

    nodeEntry->state = NodeEvalState::Invalid;
    for (PortDataItem& item : nodeEntry->outputPortData)
    {
        item.data.ptr = {};
        item.data.state = PortDataState::Outdated;
    }
    // TODO: emit that node state updated?

    Impl::propagate(*this, nodeUuid, &GraphDataModel::setNodeEvaluationFailed);
}

void
GraphDataModel::setNodeEvaluationOutdated(const NodeUuid& nodeUuid)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent())
            gtError() << tr("failed to update state of node '%1'!")
                             .arg(nodeUuid);
        return;
    }

    if (nodeEntry->state == NodeEvalState::Outdated)
    {
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Marking the data of the node '%1' as outdated")
                   .arg(nodeUuid);

    nodeEntry->state = NodeEvalState::Outdated;
    for (PortDataItem& item : nodeEntry->outputPortData)
    {
        item.data.ptr = {};
        item.data.state = PortDataState::Outdated;
    }
    // TODO: emit that node state updated?

    Impl::propagate(*this, nodeUuid, &GraphDataModel::setNodeEvaluationOutdated);
}

GraphUserVariables const*
GraphDataModel::userVariables() const
{
    auto const* root = graph().rootGraph();
    assert(root);

    return root->findDirectChild<GraphUserVariables const*>();
}

GtObject*
GraphDataModel::scope()
{
    return pimpl->scope;
}

void
GraphDataModel::setScope(GtObject& scope)
{
    if (!isSilent())
        gtTrace().verbose()
            << tr("Setting the scope of the data model of '%1' to '%2'")
                   .arg(relativeNodePath(graph()), toString(&scope));

    pimpl->scope = &scope;
}

void
GraphDataModel::setupConnections(Graph& graph)
{
    graph.disconnect(this);

    connect(&graph, &Graph::graphAboutToBeDeleted,
            this, &GraphDataModel::onGraphDeleted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodeAppended,
            this, &GraphDataModel::onNodeAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::childNodeAboutToBeDeleted,
        this, [this, g = &graph](NodeId nodeId){
            onNodeDeleted(g, nodeId);
        }, Qt::DirectConnection);

    connect(&graph, &Graph::nodePortInserted,
            this, &GraphDataModel::onNodePortInserted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodePortAboutToBeDeleted,
            this, &GraphDataModel::onNodePortDeleted,
            Qt::DirectConnection);

    for (Node* node : graph.nodes())
    {
        onNodeAppended(node);
    }
}

void
GraphDataModel::onNodeAppended(Node* node)
{
    assert(node);

    NodeUuid const nodeUuid = node->uuid();

    if (pimpl->data.contains(nodeUuid))
    {
        gtError()
            << tr("Failed to update the data model, "
                  "node '%1' (%2) already exists!")
                   .arg(relativeNodePath(*node), nodeUuid);
        return;
    }

    QVector<PortDataItem> portData;
    for (auto& portType : { PortType::Out })
    {
        for (auto& port : node->ports(portType))
        {
            assert(std::none_of(portData.cbegin(), portData.cend(),
                                [portId = port.id()](PortDataItem const& item) {
                return item.portId == portId;
            }));

            portData.push_back(PortDataItem{ port.id(), NodeDataSet{} });
        }
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updated data model: added node '%1' (%2), data size: %3")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(portData.size());

    pimpl->data.insert(nodeUuid, NodeDataItem{portData, NodeEvalState::Outdated});
    exec::setNodeDataInterface(*node, this);

    if (Graph* subgraph = qobject_cast<Graph*>(node))
    {
        setupConnections(*subgraph);
    }
}

void
GraphDataModel::onNodeDeleted(Graph* graph, NodeId nodeId)
{
    assert(nodeId.isValid());
    assert(graph);
    Node* node = graph->findNode(nodeId);
    if (!node)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "deleted node '%1' not found in '%2'!")
                   .arg(nodeId)
                   .arg(relativeNodePath(*graph));
        return;
    }

    bool success = pimpl->data.remove(node->uuid()) > 0;
    if (!success)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "deleted node '%1' (%2) not found in the data model!")
                   .arg(relativeNodePath(*node), node->uuid());
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updated data model: deleted node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid());

    // TODO: need to update successors?
    Impl::propagate(*this, node->uuid(), &GraphDataModel::setNodeEvaluationOutdated);
}
void
GraphDataModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
    if (type == PortType::In) return;

    auto* graph = qobject_cast<Graph*>(sender());
    if (!graph)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the source graph could not be determined!");
        return;
    }

    assert(nodeId.isValid());
    assert(type != invalid<PortType>());
    assert(idx.isValid());

    Node* node = graph->findNode(nodeId);
    if (!node)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the node '%1' was not found in '%2'!")
                   .arg(nodeId)
                   .arg(relativeNodePath(*graph));
        return;
    }

    NodeUuid const& nodeUuid = node->uuid();
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry != pimpl->data.end())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the node '%1' (%2) was not found in the data model!")
                   .arg(relativeNodePath(*node), node->uuid());
        return;
    }

    PortId portId = node->portId(type, idx);
    if (!portId.isValid())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "invalid port '%3' (%4) of node '%1' (%2)!")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(idx)
                   .arg(toString(type));
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updated data model: added port '%3' to node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid(), toString(*node->port(portId)));

    nodeEntry->outputPortData.append(PortDataItem{portId, NodeDataSet{}});

    // TODO: need to update successors?
    nodeEntry->state = NodeEvalState::Outdated;
    Impl::propagate(*this, node->uuid(), &GraphDataModel::setNodeEvaluationOutdated);
}

void
GraphDataModel::onNodePortDeleted(NodeId nodeId, PortType type, PortIndex idx)
{
    if (type == PortType::In) return;

    auto* graph = qobject_cast<Graph*>(sender());
    if (!graph)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the source graph could not be determined!");
        return;
    }

    assert(nodeId.isValid());
    assert(type != invalid<PortType>());
    assert(idx.isValid());

    Node* node = graph->findNode(nodeId);
    if (!node)
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the node '%1' was not found in '%2'!")
                   .arg(nodeId)
                   .arg(relativeNodePath(*graph));
        return;
    }

    NodeUuid const& nodeUuid = node->uuid();
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry != pimpl->data.end())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "the node '%1' (%2) was not found in the data model!")
                   .arg(relativeNodePath(*node), node->uuid());
        return;
    }

    PortId portId = node->portId(type, idx);
    if (!portId.isValid())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "invalid port '%3' (%4) of node '%1' (%2)!")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(idx)
                   .arg(toString(type));
        return;
    }

    auto iter = nodeEntry->outputPortData.erase(
        std::remove_if(nodeEntry->outputPortData.begin(),
                       nodeEntry->outputPortData.end(),
                       [portId](PortDataItem& item){
            return item.portId == portId;
        }), nodeEntry->outputPortData.end()
    );
    if (iter == nodeEntry->outputPortData.end())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "deleted port '%3' (%4) of node '%1' (%2) not found!")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(idx)
                   .arg(toString(type));
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updated data model: removed port '%3' from node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid(), toString(*node->port(portId)));

    // TODO: need to update successors?
    nodeEntry->state = NodeEvalState::Outdated;
    Impl::propagate(*this, node->uuid(), &GraphDataModel::setNodeEvaluationOutdated);
}

void
GraphDataModel::onGraphDeleted()
{
    Graph* graph = qobject_cast<Graph*>(sender());
    if (!graph)
    {
        gtError()
            << tr("Failed to update the data model,"
                  "a graph node has been deleted but its object was not found!");
        return;
    }

    assert(graph);
    graph->disconnect(this);

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updating the data model: removing graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());

    auto const& nodes = graph->nodes();
    for (auto* node : nodes)
    {
        onNodeDeleted(graph, node->id());
    }

    if (!isSilent())
        gtTrace().verbose()
            << tr("Updated the data model: removed graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());

    // TODO: need to update successors?
    if (graph->rootGraph() != &this->graph())
    {
        Impl::propagate(*this, graph->uuid(), &GraphDataModel::setNodeEvaluationOutdated);
    }
}
