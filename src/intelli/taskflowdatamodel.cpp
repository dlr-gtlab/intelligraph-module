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
#include "intelli/utilities.h"

#include <gt_coreapplication.h>
#include <gt_project.h>

#include <QPointer>
#include <QMutex>
#include <QMutexLocker>

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
    Impl(Graph& g) : graph(&g) {}

    QPointer<Graph> graph;
    QPointer<GtObject> scope = {};

    QHash<NodeUuid, QVector<PortDataItem>> data = {};

    QMutex mutex{};

    bool silent = false;

    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const
    {
        auto nodeEntry = data.constFind(nodeUuid);
        if (nodeEntry == data.constEnd())
        {
            if (!silent)
                gtError() << tr("failed to find data for node '%1'!")
                                 .arg(nodeUuid);
            return {};
        }

        auto portEntry = std::find_if(nodeEntry->cbegin(), nodeEntry->cend(),
                                      [portId](PortDataItem const& item){
            return item.portId == portId;
        });
        if (portEntry == nodeEntry->cend())
        {
            if (!silent)
                gtError() << tr("failed to find data for port '%2' of node '%1'!")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return {};
        }

        if (!silent)
            gtDebug()
                << tr("Accessing node data of '%1', data: %2")
                       .arg(nodeUuid, toString(portEntry->data.ptr));

        return portEntry->data;
    }

    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet dset)
    {
        auto nodeEntry = data.find(nodeUuid);
        if (nodeEntry == data.end())
        {
            if (!silent)
                gtError() << tr("failed to set data for node '%1'!")
                                 .arg(nodeUuid);
            return false;
        }

        auto portEntry = std::find_if(nodeEntry->begin(), nodeEntry->end(),
                                      [portId](PortDataItem const& item){
            return item.portId == portId;
        });
        if (portEntry == nodeEntry->end())
        {
            if (!silent)
                gtError() << tr("failed to set data for port '%2' of node '%1'!")
                                 .arg(nodeUuid)
                                 .arg(portId);
            return false;
        }

        if (!silent)
            gtDebug()
                << tr("Setting node data for '%1', data: %2")
                       .arg(nodeUuid, toString(dset.ptr));

        portEntry->data = std::move(dset);
        return true;
    }
};

TaskFlowDataModel::TaskFlowDataModel(Graph& graph) :
    pimpl(std::make_unique<Impl>(graph))
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
    if (!isSilent())
        gtDebug()
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

RunContext
TaskFlowDataModel::createRunContext(NodeUuid const& nodeUuid)
{
    if (!isSilent())
        gtDebug() << tr("Creating run context for node '%1'...")
                         .arg(nodeUuid);

    QMutexLocker locker{&pimpl->mutex};

    // TODO: graph may change during this function!

    auto& conModel = graph().globalConnectionModel();

    auto entry = conModel.find(nodeUuid);
    assert(entry != conModel.end());

    Node* node = entry->node;
    assert(node);

    RunContext context;
    context.m_caption = node->caption();

    for (PortType type : { PortType::In/*, PortType::Out*/ })
    {
        for (auto const& connection : entry->iterate(type))
        {
            // TODO: data may be changed by other thread -> fine grained locking needed
            context.m_map[connection.sourcePort] = nodeData(connection.node, connection.port).ptr;
        }
    }

    if (!isSilent())
        gtDebug() << tr("Context created for node '%1'!")
                         .arg(nodeUuid);

    return context;
}

void
TaskFlowDataModel::mergeRunResults(NodeUuid const& nodeUuid,
                                   RunResult const& results)
{
    if (!isSilent())
        gtDebug() << tr("Merging run results of node '%1'...")
                         .arg(nodeUuid);

    QMutexLocker locker{&pimpl->mutex};

    for (auto entry : utils::makeIterable(results.m_map.keyValueBegin(), results.m_map.constKeyValueEnd()))
    {
        if (results.success)
        {
            setNodeData(nodeUuid, entry.first, NodeDataSet{entry.second});
        }
        else
        {
            setNodeData(nodeUuid, entry.first, NodeDataSet{});
        }
    }

    if (!isSilent())
        gtDebug() << tr("Results merged for node '%1'!")
                         .arg(nodeUuid);
}

NodeDataSet
TaskFlowDataModel::nodeData(Graph const& graph,
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
            gtError() << tr("failed to access data for node '%1' in graph '%2', port %3 not found!")
                             .arg(nodeId)
                             .arg(relativeNodePath(graph))
                             .arg(portId);
        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataSet
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
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
            gtError() << tr("failed to access data for node '%1' in graph '%2', port %3 not found!")
                             .arg(nodeUuid, relativeNodePath(graph()))
                             .arg(portId);
        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataSet
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
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
            gtError() << tr("failed to access data for node '%1', port %2 (%3) not found!")
                             .arg(nodeUuid)
                             .arg(portIdx)
                             .arg(toString(type));
        return {};
    }

    return pimpl->nodeData(node->uuid(), portId);
}

NodeDataPtrList
TaskFlowDataModel::nodeData(NodeUuid const& nodeUuid,
                            PortType type) const
{
    assert(false); // TODO: function not needed
    assert(type == PortType::Out);

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
TaskFlowDataModel::setNodeData(Graph const& graph,
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

    return pimpl->setNodeData(node->uuid(), portId, std::move(data));
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
                               PortId portId,
                               NodeDataSet data)
{
    return pimpl->setNodeData(nodeUuid, portId, std::move(data));
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
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

    return pimpl->setNodeData(node->uuid(), portId, std::move(data));
}

bool
TaskFlowDataModel::setNodeData(NodeUuid const& nodeUuid,
                               PortType type,
                               NodeDataPtrList const& data)
{
    assert(false); // TODO: function not needed
    assert(type == PortType::Out);

    for (auto const& entry : data)
    {
        PortId portId = entry.first;
        if (!pimpl->setNodeData(nodeUuid, portId, std::move(entry.second)))
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
    // TODO, needed?

    auto nodeEntry = pimpl->data.find(nodeUuid);
    if (nodeEntry == pimpl->data.end())
    {
        if (!isSilent())
            gtError() << tr("failed to invalidate node '%1'!")
                             .arg(nodeUuid);
        return;
    }

    // TODO: use nodeEvalState here for a better indicator?
    bool alreadyProcessed = std::all_of(nodeEntry->cbegin(), nodeEntry->cend(),
                                        [](PortDataItem const& item){
        return !!item.data;
    });
    if (!alreadyProcessed) return;

    if (!isSilent())
        gtDebug()
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
    if (!isSilent())
        gtDebug()
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
    for (auto& portType : { /*PortType::In, */PortType::Out })
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
        gtDebug()
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

    if (!isSilent())
        gtDebug()
            << tr("Updated data model: deleted node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid());
}

void
TaskFlowDataModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
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
        gtDebug()
            << tr("Updated data model: added port '%3' to node '%1' (%2)")
                   .arg(relativeNodePath(*node), node->uuid(), toString(*node->port(portId)));

    nodeEntry->append(PortDataItem{portId, NodeDataSet{}});
}

void
TaskFlowDataModel::onNodePortDeleted(NodeId nodeId, PortType type, PortIndex idx)
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

    if (!isSilent())
        gtDebug()
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

    if (!isSilent())
        gtDebug()
            << tr("Updating the data model: removing graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());

    auto const& nodes = graph->nodes();
    for (auto* node : nodes)
    {
        onNodeDeleted(graph, node->id());
    }

    if (!isSilent())
        gtDebug()
            << tr("Updated the data model: removed graph '%1' (%2)")
                   .arg(relativeNodePath(*graph), graph->uuid());
}
