/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/graphexecutor.h"

#include <intelli/graph.h>
#include <intelli/graphdatamodel.h>
#include <intelli/private/utils.h>

#include <gt_algorithms.h>
#include <gt_mpl.h>

#include <QPointer>

using namespace intelli;

template<typename Functor>
struct FunctorProxy
{
    using ftraits = gt::mpl::function_traits<Functor>;
    using value_type = typename ftraits::return_type;
    using reference  = value_type;
    using pointer    = value_type;

    Functor functor = {};

    template <typename Iter>
    void init(Iter& i) { }

    template <typename Iter>
    reference get(Iter& i) { return functor(*i); }

    template <typename Iter>
    void advance(Iter& i) { ++i; }
};

template<typename Iter, typename Functor>
auto wrap(Iter* begin, Iter* end, Functor f)
{
    return intelli::makeProxy(begin, end, FunctorProxy<Functor>{f});
}

template<typename Iterable, typename Functor>
auto wrap(Iterable iter, Functor f)
{
    return intelli::makeProxy(iter, FunctorProxy<Functor>{f});
}

auto logNodePaths(QVector<NodeId> const& nodeIds, Graph const& graph)
{
    return [&nodeIds, &graph](gt::log::Stream& s) -> gt::log::Stream& {
        auto wrapped = wrap(nodeIds, [&graph](NodeId nodeId){
            Node const* node = graph.findNode(nodeId);
            return node ? relativeNodePath(*node) : QStringLiteral("NULL");
        });
        return s << gt::log::range(wrapped);
    };
}

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
    bool isProcessingPending = false;
    bool isProcessingAutoEval = false;

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
        for (NodeId const& nodeId : qAsConst(targets))
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
        return evalState(node) == NodeEvalState::Valid && !evaluating.contains(node.uuid());
    }

    inline bool isEvaluating(Node& node) const
    {
        return evaluating.contains(node.uuid());
    }

    inline bool isPaused(Node& node) const
    {
        return !node.isActive();
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
    auto const isLeafNode = [this](ConnectionData<NodeId> const& data){
        assert(data.node);
        return data.ports(PortType::Out).empty() &&
               !pimpl->targets.contains(data.node->id());
    };
    // transformer
    auto const getNodeId = [](ConnectionData<NodeId> const& data){
        assert(data.node);
        return data.node->id();
    };

    ConnectionModel const& model = graph().connectionModel();
    utils::transform_if(
        model, isLeafNode, std::back_inserter(pimpl->targets), getNodeId);

    if (graph().isBeingModified()) return {};

    pimpl->rescheduleTargetNodes();

    queuePending();

    return Future{};
}

Future
GraphExecutor::evaluateNode(NodeId nodeId)
{
    auto logError = utils::logIds(this, graph(), tr("Evaluation failed: "));

    Node* node = graph().findNode(nodeId);
    if (!node)
    {
        if (!isSilent())
            gtWarning().verbose()
                << logError << tr("invalid node '%1'!").arg(toString(nodeId));
        return {};
    }

    if (!pimpl->targets.contains(nodeId))
    {
        pimpl->targets.append(nodeId);
    }

    if (graph().isBeingModified()) return {};

    pimpl->rescheduleTargetNodes();

    queuePending();

    return {};
}

void
GraphExecutor::autoEvaluate(bool enable)
{
    pimpl->autoEvaluate = enable;

    if (!pimpl->autoEvaluate) return;

    if (graph().isBeingModified()) return;

    if (pimpl->isProcessingAutoEval) return;

    pimpl->isProcessingAutoEval = true;
    auto unlock = gt::finally([this](){ pimpl->isProcessingAutoEval = false; });
    Q_UNUSED(unlock);

    auto logError = utils::logIds(this, graph(), tr("Auto Evaluation failed:"));
    auto logTrace = utils::logIds(this, graph(), tr("Auto Evaluation updated:"));

    // if operator
    auto const isInputNode = [](ConnectionData<NodeId> const& data){
        return data.ports(PortType::In).empty();
    };
    // transformer
    auto const getNodeId = [](ConnectionData<NodeId> const& data){
        assert(data.node);
        return data.node->id();
    };

    // find starting nodes
    QVector<NodeId> nextNodeIds;
    auto const& conModel = graph().connectionModel();
    utils::transform_if(
        conModel, isInputNode, std::back_inserter(nextNodeIds), getNodeId);

    int readyCount = 0;
    bool queued = false;
    while (!nextNodeIds.empty())
    {
        NodeId nodeId = nextNodeIds.takeLast();

        Node* node = graph().findNode(nodeId);
        if (!node)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << logError << tr("invalid node '%1'!").arg(toString(nodeId));
            break;
        }

        auto logErrorExt = utils::logIds(logError, node, "->");
        auto logTraceExt = utils::logIds(logTrace, node, "->");

        if (pimpl->isEvaluated(*node))
        {
            if (!isSilent())
                gtTrace().verbose() << logTraceExt << tr("node is already evaluated!");

            readyCount++;

            // node evaluated -> schedule next nodes
            auto nextIter = conModel.iterateNodes(nodeId, PortType::Out);
            for (NodeId next : nextIter)
            {
                if (!nextNodeIds.contains(next)) nextNodeIds.push_front(next);
            }
            continue;
        }

        if (pimpl->isPaused(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("node is paused!");
            continue;
        }

        if (pimpl->isEvaluating(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("node is already evaluating!");
            continue;
        }

        // node not yet evaluated
        if (!pimpl->areDependenciesMet(*node))
        {
            if (!isSilent())
                gtWarning().verbose() << logErrorExt << tr("dependencies not met!");
            break;
        }

        if (!isSilent())
            gtTrace().verbose()
                << logTraceExt << tr("node queued!");

        pimpl->queue.push_front(nodeId);
        queued = true;
    }

    if (readyCount == conModel.size())
    {
        if (!isSilent())
            gtTrace().verbose()
                << logTrace << tr("all nodes evaluated!");

        assert(!queued);
        emit allNodesEvaluated();
        return;
    }

    if (queued)
    {
        evaluateQueue();
    }
}

void
GraphExecutor::queuePending()
{
    if (graph().isBeingModified()) return;

    if (pimpl->isProcessingPending) return;

    pimpl->isProcessingPending = true;
    auto unlock = gt::finally([this](){ pimpl->isProcessingPending = false; });
    Q_UNUSED(unlock);

    auto logError = utils::logIds(this, graph(), tr("Queuing Pending Nodes failed:"));
    auto logTrace = utils::logIds(this, graph(), tr("Pending Nodes updated:"));

    QVector<NodeId>& pending = pimpl->queue;

    bool failure = false;
    bool queued = false;
    for (int idx = 0; idx < pending.size(); idx++)
    {
        auto removeFromPending = gt::finally([&pending, &idx](){
            pending.removeAt(idx);
            idx--;
        });
        Q_UNUSED(removeFromPending);

        NodeId nodeId = pending.at(idx);
        Node* node = graph().findNode(nodeId);
        if (!node)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << logError << tr("invalid node '%1'!").arg(toString(nodeId));
            failure = true;
            break;
        }

        auto logErrorExt = utils::logIds(logError, node, "->");
        auto logTraceExt = utils::logIds(logTrace, node, "->");

        if (pimpl->isEvaluated(*node))
        {
            if (!isSilent())
                gtTrace().verbose() << logTraceExt << tr("node is already evaluated!");
            continue;
        }

        if (pimpl->isEvaluating(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("node is already evaluating!");
            continue;
        }

        removeFromPending.clear();

        // node not yet evaluated
        if (!pimpl->areDependenciesMet(*node))
        {
            if (!isSilent())
                gtWarning().verbose() << logErrorExt << tr("dependencies not met!");
            break;
        }

        if (!isSilent())
            gtTrace().verbose()
                << logTraceExt << tr("node queued!");

        pimpl->queue.push_front(nodeId);
        queued = true;
    }

    if (failure)
    {
        if (!isSilent())
            gtTrace().verbose()
                << logTrace << tr("failed to evaluate some nodes!");

        return;
    }

    if (queued)
    {
        evaluateQueue();
    }

}

void
GraphExecutor::evaluateQueue()
{
    if (graph().isBeingModified()) return;

    if (pimpl->isProcessingQueue) return;

    pimpl->isProcessingQueue = true;
    auto unlock = gt::finally([this](){ pimpl->isProcessingQueue = false; });
    Q_UNUSED(unlock);

    if (pimpl->queue.empty()) return;

    QVector<NodeId>& queue = pimpl->queue;

    auto logError = utils::logIds(this, graph(), tr("Evaluating Queue failed: "));
    auto logTrace = utils::logIds(this, graph(), tr("Queue updated:"));

    QVector<NodeId> backlog;
    auto appendBacklog = gt::finally([this, &backlog, &queue, &logTrace](){
        if (backlog.empty()) return;

        if (!isSilent())
            gtTrace().verbose()
                << logTrace << tr("Merging backlog:") << backlog << "...";

        std::copy(backlog.begin(), backlog.end(), std::back_inserter(queue));
    });
    Q_UNUSED(appendBacklog);

    if (!isSilent())
        gtTrace().verbose()
            << logTrace << tr("evaluating queue:") << logNodePaths(pimpl->queue, graph());

    while(!queue.empty())
    {
        NodeId nodeId = queue.takeLast();

        assert(!pimpl->pending.contains(nodeId));

        Node* node = graph().findNode(nodeId);
        if (!node)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << logError << tr("invalid node '%1'!").arg(toString(nodeId));
            continue;
        }

        auto logErrorExt = utils::logIds(logError, node, "->");
        auto logTraceExt = utils::logIds(logTrace, node, "->");

        if (!isSilent())
            gtTrace().verbose()
                << logTraceExt << tr("triggering evaluation...");

        if (auto* subgraph = qobject_cast<Graph*>(node))
        {
            if (subgraph->isBeingModified())
            {
                if (!isSilent())
                    gtTrace().verbose()
                        << logErrorExt << tr("node is being modifed!");
                continue;
            }
        }

        if (pimpl->isEvaluating(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("node is already evaluating!");
            continue;
        }

        if (!pimpl->areDependenciesMet(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("dependencies not met!");
            continue;
        }

        if (!pimpl->areInputsReady(*node))
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logErrorExt << tr("inputs not ready!");
            continue;
        }

        // TODO: move exclusive logic to node class
        if ((size_t)node->nodeEvalMode() & IsExclusiveMask)
        {
            gtWarning()
                << tr("exclusive node evaluation is not handled currently!");
        }
        assert(exec::nodeDataInterface(*node) == pimpl->dataModel);

        if (!exec::triggerNodeEvaluation(*node))
        {
            if (!isSilent())
                gtError().verbose()
                    << logErrorExt << tr("triggering evaluation failed!");
            continue;
        }

        if (!isSilent())
            gtTrace().verbose()
                << logTraceExt << tr("triggering evaluation succeeded!");
    }
}


void
GraphExecutor::onNodeEvaluationStarted(QString const& nodeUuid)
{
    if (Node const* node = graph().findNodeByUuid(nodeUuid))
    {
        if (Graph::accessGraph(*node) != &graph()) return;
    }

    auto logTrace = utils::logIds(this, graph());

    if (!isSilent())
        gtTrace().verbose()
            << logTrace << tr("node evaluation of '%1' started!")
                                          .arg(nodeUuid);

    assert(!pimpl->evaluating.contains(nodeUuid));
    pimpl->evaluating.push_back(nodeUuid);
}

void
GraphExecutor::onNodeEvaluationFinished(QString const& nodeUuid)
{
    if (Node const* node = graph().findNodeByUuid(nodeUuid))
    {
        if (Graph::accessGraph(*node) != &graph()) return;
    }

    auto logTrace = utils::logIds(this, graph());

    if (!isSilent())
        gtTrace().verbose()
            << logTrace << tr("node evaluation of '%1' finished!")
                               .arg(nodeUuid);

    QTimer::singleShot(0, this, std::bind(&GraphExecutor::onNodeEvaluated, this, nodeUuid));
}

void
GraphExecutor::onNodeEvaluated(const NodeUuid& nodeUuid)
{
    assert(pimpl->evaluating.contains(nodeUuid));
    pimpl->evaluating.removeOne(nodeUuid);

    auto logError = utils::logIds(this, graph(), tr("Finalizing evaluation failed: "));
    auto logTrace = utils::logIds(this, graph(), tr("Finalizing evaluation: "));

    Node* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        gtError() << logError << tr("invalid node '%1'!").arg(nodeUuid);
        return;
    }

    bool isValidNode = graph().findNode(node->id()) == node;
    if (!isValidNode)
    {
        gtError() << logError << tr("unknown node '%1'!").arg(relativeNodePath(*node));
        return;
    }

    if (!isSilent())
        gtTrace().verbose()
            << logTrace << tr("node '%1'!").arg(relativeNodePath(*node));

    NodeEvalState state = pimpl->evalState(*node);
    switch (state)
    {
    case NodeEvalState::Invalid:
        gtError() << logError << tr("execution failed?");
        return;
    case NodeEvalState::Outdated:
        gtError() << logError << tr("outdated, was invalidated and must restart?");
        return;
    case NodeEvalState::Evaluating:
        gtError() << logError << tr("still evaluating?");
        return;
    case NodeEvalState::Paused:
        gtError() << logError << tr("paused?");
        return;
    case NodeEvalState::Valid:
        break;
    }

    if (pimpl->autoEvaluate)
    {
        autoEvaluate();
    }

    bool isTarget = pimpl->targets.contains(node->id());
    if (isTarget)
    {
        pimpl->targets.removeOne(node->id());

        if (pimpl->targets.empty())
        {
            if (!isSilent())
                gtTrace().verbose()
                    << logTrace << tr("target nodes evaluated!");

            assert(pimpl->pending.empty());
            emit targetNodesEvaluated();
        }
    }

    if (pimpl->pending.size() > 0)
    {
        queuePending();
    }


    if (!isSilent())
        gtTrace().verbose()
            << logTrace << tr("node finalized!");
}

