/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/taskflowdatamodel.h"
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

struct TaskFlowDataModel::Impl
{
    QPointer<Graph> graph;
    QPointer<GtObject> scope = {};

    QHash<NodeUuid, QVector<PortDataItem>> data = {};

    bool silent = false;
};

TaskFlowDataModel::TaskFlowDataModel(Graph& graph) :
    pimpl(std::make_unique<Impl>(Impl{&graph}))
{
    setParent(&graph);

    setObjectName(QStringLiteral("__data_model"));

    if (gtApp) pimpl->scope = gtApp->currentProject();

    reset();
}

TaskFlowDataModel::~TaskFlowDataModel() = default;

bool
TaskFlowDataModel::isSilent() const
{
    return pimpl->silent;
}

void
TaskFlowDataModel::setSilent(bool value)
{
    pimpl->silent = value;
}

void
TaskFlowDataModel::reset()
{
    if (!isSilent()) gtDebug()
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
TaskFlowDataModel::graph()
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

Graph const&
TaskFlowDataModel::graph() const
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

NodeDataSet
TaskFlowDataModel::nodeData(Graph const& graph,
                            NodeId nodeId,
                            PortId portId) const
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        if (!isSilent()) gtError() << tr("failed to find data for node '%1' in graph '%2'!")
                             .arg(nodeId)
                             .arg(relativeNodePath(graph));
        return {};
    }

    return nodeData(node->uuid(), portId);
}

NodeDataSet
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
                            PortId portId) const
{
    auto nodeEntry = pimpl->data.constFind(nodeUuid);
    if (nodeEntry == pimpl->data.constEnd())
    {
        if (!isSilent()) gtError() << tr("failed to find data for node '%1'!")
                             .arg(nodeUuid);
        return {};
    }

    auto portEntry = std::find_if(nodeEntry->cbegin(), nodeEntry->cend(),
                                  [portId](PortDataItem const& item){
        return item.portId == portId;
    });
    if (portEntry == nodeEntry->cend())
    {
        if (!isSilent()) gtError() << tr("failed to find data for port '%2' of node '%1'!")
                             .arg(nodeUuid)
                             .arg(portId);
        return {};
    }

    if (!isSilent()) gtDebug()
            << tr("Accessing node data of '%1', data: %2")
                   .arg(nodeUuid, toString(portEntry->data.ptr));

    return portEntry->data;
}

NodeDataSet
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
                            PortType type,
                            PortIndex portIdx) const
{
    Node const* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        if (!isSilent()) gtError() << tr("failed to find data for node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    PortId portId = node->portId(type, portIdx);
    if (!portId.isValid())
    {
        if (!isSilent()) gtError() << tr("failed to find data for node '%1': invalid port %2 (%3)!")
                             .arg(nodeUuid)
                             .arg(portIdx)
                             .arg(toString(type));
        return {};
    }

    return nodeData(node->uuid(), portId);
}

NodeDataPtrList
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
                            PortType type) const
{
    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        if (!isSilent()) gtError() << tr("failed to find data for node '%1' in graph '%2'!")
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
TaskFlowDataModel::setNodeData(Graph const& graph,
                               NodeId nodeId,
                               PortId portId,
                               NodeDataSet data)
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        if (!isSilent()) gtError() << tr("failed to set data for node '%1' in graph '%2'!")
                             .arg(nodeId)
                             .arg(relativeNodePath(graph));
        return {};
    }

    return setNodeData(node->uuid(), portId, std::move(data));
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
                               PortId portId,
                               NodeDataSet data)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent()) gtError() << tr("failed to set data for node '%1'!")
                             .arg(nodeUuid);
        return false;
    }

    auto portEntry = std::find_if(nodeEntry->begin(), nodeEntry->end(),
                                  [portId](PortDataItem const& item){
                                      return item.portId == portId;
                                  });
    if (portEntry == nodeEntry->end())
    {
        if (!isSilent()) gtError() << tr("failed to set data for port '%2' of node '%1'!")
                             .arg(nodeUuid)
                             .arg(portId);
        return false;
    }

    if (!isSilent()) gtDebug()
            << tr("Setting node data for '%1', data: %2")
                   .arg(nodeUuid, toString(data.ptr));

    portEntry->data = std::move(data);
    return true;
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
                               PortType type,
                               PortIndex portIdx,
                               NodeDataSet data)
{
    Node const* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        if (!isSilent()) gtError() << tr("failed to set data for node '%1' in graph '%2'!")
                             .arg(nodeUuid, relativeNodePath(graph()));
        return {};
    }

    PortId portId = node->portId(type, portIdx);
    if (!portId.isValid())
    {
        if (!isSilent()) gtError() << tr("failed to set data for node '%1': invalid port %2 (%3)!")
                             .arg(nodeUuid)
                             .arg(portIdx)
                             .arg(toString(type));
        return {};
    }

    return setNodeData(node->uuid(), portId, std::move(data));
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
                               PortType type,
                               NodeDataPtrList const& data)
{
    for (auto const& entry : data)
    {
        PortId portId = entry.first;
        if (!setNodeData(nodeUuid, portId, std::move(entry.second)))
        {
            if (!isSilent()) gtError() << tr("failed to set data for port '%2' of node '%1'!")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return false;
        }
    }

    return true;
}

NodeEvalState
TaskFlowDataModel::nodeEvalState(NodeUuid const& nodeUuid) const
{
    // TODO
    return NodeEvalState::Invalid;
}

void
TaskFlowDataModel::nodeEvaluationStarted(NodeUuid const& nodeUuid)
{
    // TODO
}

void
TaskFlowDataModel::nodeEvaluationFinished(NodeUuid const& nodeUuid)
{
    // TODO
}

void
TaskFlowDataModel::setNodeEvaluationFailed(NodeUuid const& nodeUuid)
{
    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent()) gtError() << tr("failed to invalidate node '%1'!")
                             .arg(nodeUuid);
        return;
    }

    // TODO: use nodeEvalState here for a better indicator?
    bool alreadyProcessed = std::all_of(nodeEntry->cbegin(), nodeEntry->cend(),
                                        [](PortDataItem const& item){
        return !!item.data;
    });
    if (!alreadyProcessed) return;

    if (!isSilent()) gtDebug()
            << tr("Marking the data of the node '%1' as invalid")
                   .arg(nodeUuid);

    for (PortDataItem& item : *nodeEntry)
    {
        item.data.ptr = {};
        item.data.state = PortDataState::Outdated;
    }

    // TODO: need to update successors?
    for (auto& successor : graph().globalConnectionModel().iterateNodes(nodeUuid, PortType::Out))
    {
        NodeDataInterface::setNodeEvaluationFailed(successor);
    }
}

GraphUserVariables const*
TaskFlowDataModel::userVariables() const
{
    auto const* root = graph().rootGraph();
    assert(root);

    return root->findDirectChild<GraphUserVariables const*>();
}

GtObject*
TaskFlowDataModel::scope()
{
    return pimpl->scope;
}

void
TaskFlowDataModel::setScope(GtObject& scope)
{
    if (!isSilent()) gtDebug()
            << tr("Setting the scope of the data model of '%1' to '%2'")
                   .arg(relativeNodePath(graph()), toString(&scope));

    pimpl->scope = &scope;
}

void
TaskFlowDataModel::setupConnections(Graph& graph)
{
    graph.disconnect(this);

    connect(&graph, &Graph::graphAboutToBeDeleted,
            this, &TaskFlowDataModel::onGraphDeleted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodeAppended,
            this, &TaskFlowDataModel::onNodeAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::childNodeAboutToBeDeleted,
        this, [this, g = &graph](NodeId nodeId){
            onNodeDeleted(g, nodeId);
        }, Qt::DirectConnection);

    connect(&graph, &Graph::globalConnectionAppended,
            this, &TaskFlowDataModel::onConnectionAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::globalConnectionDeleted,
            this, &TaskFlowDataModel::onConnectionDeleted,
            Qt::DirectConnection);

    connect(&graph, &Graph::nodePortInserted,
            this, &TaskFlowDataModel::onNodePortInserted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodePortAboutToBeDeleted,
            this, &TaskFlowDataModel::onNodePortDeleted,
            Qt::DirectConnection);
}

void
TaskFlowDataModel::onNodeAppended(Node* node)
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
    for (auto& portType : { PortType::In, PortType::Out })
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
    if (!isSilent()) gtDebug()
            << tr("Updated data model: added node '%1' (%2), data size: %3")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(portData.size());

    pimpl->data.insert(nodeUuid, portData);
}

void
TaskFlowDataModel::onNodeDeleted(Graph* graph, NodeId nodeId)
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

    if (!isSilent()) gtDebug()
            << tr("Updated data model: deleted node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid());
}

void
TaskFlowDataModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
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

    if (!isSilent()) gtDebug()
            << tr("Updated data model: added port '%3' to node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid(), toString(*node->port(portId)));

    nodeEntry->append(PortDataItem{portId, NodeDataSet{}});
}

void
TaskFlowDataModel::onNodePortDeleted(NodeId nodeId, PortType type, PortIndex idx)
{
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

    auto iter = nodeEntry->erase(
        std::remove_if(nodeEntry->begin(), nodeEntry->end(),
                       [portId](PortDataItem& item){
            return item.portId == portId;
        }), nodeEntry->end()
    );
    if (iter == nodeEntry->end())
    {
        gtError()
            << tr("Failed to update the data model, "
                  "deleted port '%3' (%4) of node '%1' (%2) not found!")
                   .arg(relativeNodePath(*node), node->uuid())
                   .arg(idx)
                   .arg(toString(type));
        return;
    }

    if (!isSilent()) gtDebug()
            << tr("Updated data model: removed port '%3' from node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid(), toString(*node->port(portId)));
}

void
TaskFlowDataModel::onConnectionAppended(ConnectionUuid conUuid)
{
    // TODO: needed?
}

void
TaskFlowDataModel::onConnectionDeleted(ConnectionUuid conUuid)
{
    // TODO: needed?
}

void
TaskFlowDataModel::onGraphDeleted()
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

    if (!isSilent()) gtDebug()
            << tr("Updating the data model: removing graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());

    auto const& nodes = graph->nodes();
    for (auto* node : nodes)
    {
        onNodeDeleted(graph, node->id());
    }

    if (!isSilent()) gtDebug()
            << tr("Updated the data model: removed graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());
}
