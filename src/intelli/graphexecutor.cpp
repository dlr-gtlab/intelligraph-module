/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/graphexecutor.h"
#include "gt_algorithms.h"

#include <intelli/graph.h>
#include <intelli/graphdatamodel.h>
#include <intelli/private/utils.h>

#include <QPointer>

using namespace intelli;

struct GraphExecutor::Impl
{
    explicit Impl(Graph& g, GraphDataModel& d) : graph(&g), dataModel(&d) {}

    /// associated graph
    QPointer<Graph> graph;
    /// associated data model
    QPointer<GraphDataModel> dataModel;

    QVector<NodeId> queue, targets, pending;

    QVector<NodeUuid> evaluating;

    bool isProcessingQueue = false;

    bool autoEvaluate = false;

    bool silent = false;

    /// Helper method that accumulates all dependencies recursively of the given
    /// node and appens them to the spcified list.
    static void
    accumulateDependencies(ConnectionModel const& conModel,
                           QVector<NodeId>& vector,
                           NodeId const& nodeId,
                           PortType type = PortType::In)
    {
        if (vector.contains(nodeId)) return;

        vector.push_back(nodeId);
        for (NodeId nextNodeId : conModel.iterateNodes(nodeId, type))
        {
            accumulateDependencies(conModel, vector, nextNodeId, type);
        }
    }

    /// Sorts the given list according to topo sort, such that all nodes
    /// at the start of the list have no dependencies
    static void
    topoSort(ConnectionModel const& conModel, QVector<NodeId>& vector)
    {
        std::map<NodeId, QVector<NodeId>> adjacencyMatrix;
        for (NodeId nodeId : vector)
        {
            QVector<NodeId> predecessors;
            for (auto& predecessor : conModel.iterateUniqueNodes(nodeId, PortType::Out))
            {
                predecessors.push_back(predecessor);
            }
            adjacencyMatrix.insert({nodeId, predecessors});
        }
        vector = gt::topo_sort(adjacencyMatrix);
    }

    inline void
    rescheduleTargetNodes()
    {
        pending.clear();
        if (targets.empty()) return;

        auto& conModel = graph->connectionModel();

        // reschedule target nodes
        for (NodeId const& nodeId : targets)
        {
            accumulateDependencies(conModel, pending, nodeId);
        }

        topoSort(conModel, pending);

        // remove evluated nodes
        pending.erase(
            std::remove_if(pending.begin(), pending.end(), [this](NodeId nodeId){
              return dataModel->nodeEvalState(graph->nodeUuid(nodeId)) == NodeEvalState::Valid;
            }), pending.end()
        );
    }

    inline NodeEvalState evalState(Node& node) const
    {
        return dataModel->nodeEvalState(node.uuid());
    }

    inline bool isEvaluated(Node& node) const
    {
        return evalState(node) == NodeEvalState::Valid;
    }

    inline bool areDependenciesMet(Node& node) const
    {
        auto const dependencies = graph->connectionModel().iterateNodes(node.id(), PortType::In);
        bool ready =
            std::all_of(dependencies.begin(),
                        dependencies.end(),
                        [this](NodeId nodeId){
                Node* dependency = graph->findNode(nodeId);
                return dependency && isEvaluated(*dependency);
            });
        return ready;
    }

    inline bool areInputsReady(Node& node) const
    {
        auto const& inputPorts = node.ports(PortType::In);
        bool ready =
            std::all_of(inputPorts.begin(),
                        inputPorts.end(),
                        [](NodePort const& port){
                return port.isConnected() || port.optional;
            });
        return ready;
    }

}; // struct Impl

GraphExecutor::GraphExecutor(Graph& graph, GraphDataModel& dataModel) :
    QObject(&graph),
    pimpl(std::make_unique<Impl>(graph, dataModel))
{
    setObjectName(QStringLiteral("__executor"));

    reset();
}

GraphExecutor::~GraphExecutor() = default;

bool
GraphExecutor::isSilent() const
{
    return pimpl->silent;
}

void
GraphExecutor::setSilent(bool value)
{
    pimpl->silent = value;
}

Graph&
GraphExecutor::graph()
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

Graph const&
GraphExecutor::graph() const
{
    assert(pimpl->graph);
    return *pimpl->graph;
}

GraphDataModel&
GraphExecutor::dataModel()
{
    assert(pimpl->dataModel);
    return *pimpl->dataModel;
}

GraphDataModel const&
GraphExecutor::dataModel() const
{
    assert(pimpl->dataModel);
    return *pimpl->dataModel;
}

void
GraphExecutor::reset()
{
    pimpl->queue.clear();
    pimpl->pending.clear();
    pimpl->targets.clear();

    auto& dm = dataModel();
    dm.disconnect(this);
    connect(&dm, &GraphDataModel::evaluationStarted,
            this, &GraphExecutor::onNodeEvaluationStarted);
    connect(&dm, &GraphDataModel::evaluationFinished,
            this, &GraphExecutor::onNodeEvaluationFinished);
}

Future
GraphExecutor::evaluateGraph()
{
    // if operator
    auto const isLeafNode = [](ConnectionData<NodeId> const& data){
        return data.ports(PortType::Out).empty();
    };
    // transformer
    auto const getNodeId = [](ConnectionData<NodeId> const& data){
        assert(data.node);
        return data.node->id();
    };

    auto& targets = pimpl->targets;

    ConnectionModel const& model = graph().connectionModel();
    utils::transform_if(
        model, isLeafNode, std::back_inserter(targets), getNodeId);

    // remove duplicates
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

    pimpl->rescheduleTargetNodes();

//    evaluateQueue();

    return Future{};
}

void
GraphExecutor::autoEvaluate(bool enable)
{
    auto makeError = [this](){
        return utils::logId(this) + tr(" failed to auto evaluate node:");
    };

    pimpl->autoEvaluate = enable;

    if (!pimpl->autoEvaluate) return;

    if (graph().isBeingModified()) return;

    // if operator
    auto const isInputNode = [](ConnectionData<NodeId> const& data){
        return data.ports(PortType::In).empty();
    };
    // transformer
    auto const getNodeId = [](ConnectionData<NodeId> const& data){
        assert(data.node);
        return data.node->id();
    };

    QVector<NodeId> nextNodeIds;
    auto const& conModel = graph().connectionModel();
    utils::transform_if(
        conModel, isInputNode, std::back_inserter(nextNodeIds), getNodeId);

    bool allReady = true;
    bool queued = false;
    while (!nextNodeIds.empty())
    {
        NodeId nodeId = nextNodeIds.takeLast();

        Node* node = graph().findNode(nodeId);
        if (!node)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << makeError()
                    << tr("invalid node '%1'!").arg(toString(nodeId));
            break;
        }

        if (pimpl->isEvaluated(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(this)
                    << tr("node '%1' ready!").arg(node->caption());

            // node evaluated -> schedule next nodes
            auto nextIter = conModel.iterateNodes(nodeId, PortType::Out);
            for (NodeId next : nextIter)
            {
                if (!nextNodeIds.contains(next)) nextNodeIds.push_front(next);
            }
            continue;
        }

        // node not yet evaluated
        if (!pimpl->areDependenciesMet(*node))
        {
            if (!isSilent())
                gtWarning().verbose()
                    << makeError()
                    << tr("dependencies of node '%1' not ready!").arg(node->caption());

            allReady = false;
            break;
        }

        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(this)
                << tr("node '%1' queued!").arg(node->caption());

        pimpl->queue.push_front(nodeId);
        queued = true;
    }

    if (queued)
    {
        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(*this)
                << tr("queued:") << gt::log::range(pimpl->queue) << "...";
        evaluateQueue();
    }
//    if (allReady)
//    {
//        if (!isSilent())
//            gtTrace().verbose()
//                << utils::logId(this)
//                << tr("all nodes evaluated!");

//        emit allNodesEvaluated();
//    }
}

void
GraphExecutor::evaluateQueue()
{
    if (graph().isBeingModified()) return;

    if (pimpl->isProcessingQueue) return;

    pimpl->isProcessingQueue = true;
    auto unlockQueue = gt::finally([this](){ pimpl->isProcessingQueue = false; });
    Q_UNUSED(unlockQueue);

    if (pimpl->queue.empty()) return;

    QVector<NodeId>& queue = pimpl->queue;

    QVector<NodeId> backlog;
    auto appendBacklog = gt::finally([this, &backlog, &queue](){
        if (backlog.empty()) return;

        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(*this)
                << tr("backlog:") << gt::log::range(backlog) << "...";
        std::copy(backlog.begin(), backlog.end(), std::back_inserter(queue));
    });
    Q_UNUSED(appendBacklog);

    if (!isSilent())
        gtTrace().verbose()
            << utils::logId(*this)
            << tr("evaluating queue:") << gt::log::range(queue) << "...";

    while(!queue.empty())
    {
        NodeId nodeId = queue.takeLast();

        assert(!pimpl->pending.contains(nodeId));

        Node* node = graph().findNode(nodeId);
        if (!node)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << utils::logId(*this)
                    << tr("failed to evaluate queued node '%1'! (node deleted?)")
                           .arg(toString(nodeId));
            continue;
        }

        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(*this)
                << tr("evaluating node '%1'...")
                       .arg(relativeNodePath(*node));

        if (auto* subgraph = qobject_cast<Graph*>(node))
        {
            if (subgraph->isBeingModified())
            {
                if (!isSilent())
                    gtTrace().verbose()
                        << utils::logId(*this)
                        << tr("-> subgraph is being modifed.");
                continue;
            }
        }

        bool isEvaluating = pimpl->evaluating.contains(node->uuid());
        if (isEvaluating)
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> already evaluating!");
            continue;
        }

        if (!pimpl->areDependenciesMet(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> dependencies not met!");
            continue;
        }

        if (!pimpl->areInputsReady(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> inputs not met!");
            continue;
        }

        // TODO: move exclusive logic to node class
        if ((size_t)node->nodeEvalMode() & IsExclusiveMask)
        {
            gtWarning()
                << utils::logId(*this) << tr("exclusive node evaluation is not handled currently!");
        }
        assert(exec::nodeDataInterface(*node) == pimpl->dataModel);

        if (!exec::triggerNodeEvaluation(*node))
        {
            if (!isSilent())
                gtError().verbose()
                    << utils::logId(*this) << tr("-> triggering evaluation failed!");
            continue;
        }

        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(*this) << tr("-> triggering evaluation succeeded!");
    }
}


void
GraphExecutor::onNodeEvaluationStarted(QString const& nodeUuid)
{
    if (!isSilent())
        gtTrace().verbose()
            << utils::logId(*this) << tr("node evaluation of '%1' started!")
                                          .arg(nodeUuid);

    assert(!pimpl->evaluating.contains(nodeUuid));
    pimpl->evaluating.push_back(nodeUuid);
}

void
GraphExecutor::onNodeEvaluationFinished(QString const& nodeUuid)
{
    if (!isSilent())
        gtTrace().verbose()
            << utils::logId(*this) << tr("node evaluation of '%1' finished!")
                                          .arg(nodeUuid);

    assert(pimpl->evaluating.contains(nodeUuid));
    pimpl->evaluating.removeOne(nodeUuid);

    QTimer::singleShot(0, this, std::bind(&GraphExecutor::onNodeEvaluated, this, nodeUuid));
}

void
GraphExecutor::onNodeEvaluated(const NodeUuid& nodeUuid)
{
    Node* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        gtError() << utils::logId(*this)
                  << relativeNodePath(graph())
                  << tr("failed to finalize node evaluation of node '%1'! "
                        "(invalid node)")
                         .arg(nodeUuid);
        return;
    }

    bool isValidNode = graph().findNode(node->id()) == node;
    if (!isValidNode)
    {
        gtError()
            << utils::logId(*this)
            << relativeNodePath(graph())
            << tr("failed to finalize node evaluation of node '%1'! "
                  "(node deleted?)")
                   .arg(nodeUuid);
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << utils::logId(*this) << tr("finalizing node evaluation of '%1'!")
                                          .arg(relativeNodePath(*node));

    NodeEvalState state = pimpl->evalState(*node);
    switch (state)
    {
    case NodeEvalState::Invalid:
        gtError()
            << utils::logId(*this) << tr("-> execution failed?");
        return;
    case NodeEvalState::Outdated:
        gtError()
            << utils::logId(*this) << tr("-> outdated, must restart?");
        return;
    case NodeEvalState::Evaluating:
        gtError()
            << utils::logId(*this) << tr("-> evaluating?");
        return;
    case NodeEvalState::Paused:
        gtError()
            << utils::logId(*this) << tr("-> paused?");
        return;
    case NodeEvalState::Valid:
        gtTrace().verbose()
            << utils::logId(*this) << tr("-> sucess!");
        break;
    }

    if (pimpl->autoEvaluate)
    {
        auto makeError = [this](){
            return utils::logId(this) + tr(" failed to schedule next evaluate node:");
        };

        auto nextNodeIds = graph().connectionModel().iterateUniqueNodes(node->id());

        bool queued = false;
        for (NodeId nextNodeId : nextNodeIds)
        {
            Node* nextNode = graph().findNode(nextNodeId);
            if (!nextNode)
            {
                if (!isSilent())
                    gtWarning().verbose()
                        << makeError()
                        << tr("invalid node '%1'!").arg(toString(nextNodeId));
                break;
            }
            if (pimpl->isEvaluated(*nextNode))
            {
                if (!isSilent())
                    gtWarning().verbose()
                        << makeError()
                        << tr("next noode already evaluated '%1'!").arg(toString(nextNodeId));
                break;
            }
//            assert(!pimpl->isEvaluated(*nextNode));

            // node not yet evaluated
            if (!pimpl->areDependenciesMet(*nextNode))
            {
                if (!isSilent())
                    gtWarning().verbose()
                        << makeError()
                        << tr("dependencies of node '%1' not ready!").arg(nextNode->caption());
                break;
            }

            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(this)
                    << tr("node '%1' queued!").arg(nextNode->caption());

            pimpl->queue.push_front(nextNodeId);
            queued = true;
        }

        if (queued)
        {
            evaluateQueue();
        }

        if (!queued && nextNodeIds.empty())
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(this)
                    << tr("all nodes evaluated!");

            emit allNodesEvaluated();
        }
    }

    if (!isSilent())
        gtTrace().verbose()
            << utils::logId(*this) << tr("finalized node evaluation!");
}

