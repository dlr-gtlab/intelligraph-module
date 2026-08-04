/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/graphexecutor.h"

#include <intelli/graph.h>
#include <intelli/graphdatamodel.h>
#include <intelli/private/utils.h>

#include <QPointer>

using namespace intelli;

using Queue = QVector<QPointer<Node>>;

struct GraphExecutor::Impl
{
    explicit Impl(Graph& g, GraphDataModel& d) : graph(&g), dataModel(&d) {}

    /// associated graph
    QPointer<Graph> graph;
    /// associated data model
    QPointer<GraphDataModel> dataModel;

    Queue queue;

    Queue evaluating;

    bool isProcessingQueue = false;

    bool silent = false;

}; // struct Impl

GraphExecutor::GraphExecutor(Graph& graph, GraphDataModel& dataModel) :
    QObject(&graph),
    pimpl(std::make_unique<Impl>(graph, dataModel))
{
    setObjectName(QStringLiteral("__executor"));
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
}

Future
GraphExecutor::evaluateAll()
{
    // find "input" nodes
    ConnectionModel const& model = graph().connectionModel();
    utils::transform_if(
        model,
        // if operator
        [](ConnectionData<NodeId> const& data){
            return data.ports(PortType::In).empty();
        },
        std::back_inserter(pimpl->queue),
        // transformer
        [](ConnectionData<NodeId> const& data){
            assert(data.node);
            return data.node;
        }
    );

    evaluateQueue();

    return Future{};
}

void
GraphExecutor::evaluateQueue()
{
    if (graph().isBeingModified()) return;

    if (pimpl->isProcessingQueue) return;
    pimpl->isProcessingQueue = true;

    auto unlockQueue = gt::finally([this](){ pimpl->isProcessingQueue = false; });
    Q_UNUSED(unlockQueue);

    Queue& queue = pimpl->queue;

    Queue backlog;
    auto appendBacklog = gt::finally([this, &backlog, &queue](){
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
        auto node = queue.takeLast();
        if (!node)
        {
            gtError() << utils::logId(*this)
                      << tr("failed to evaluate node in queue! (invalid node");
            continue;
        }

        bool isValidNode = graph().findNode(node->id()) == node;
        if (!isValidNode)
        {
            if (!isSilent())
                gtWarning().verbose()
                    << utils::logId(*this)
                    << tr("failed to evaluate queued node '%1'! (node deleted?)")
                           .arg(relativeNodePath(*node));
            continue;
        }

        if (!isSilent())
            gtTrace().verbose()
                << utils::logId(*this)
                << tr("evaluating queued node '%1'...")
                       .arg(relativeNodePath(*node));

        if (auto* subgraph = qobject_cast<Graph*>(node))
        {
            if (subgraph->isBeingModified())
            {
                if (!isSilent())
                    gtTrace().verbose()
                        << utils::logId(*this) << tr("subgraph is being modifed.");
                continue;
            }
        }

        bool isEvaluating = pimpl->evaluating.contains(node);
        if (isEvaluating)
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> already evaluating!");
            continue;
        }

        auto const dependencies = graph().connectionModel().iterateNodes(node->id(), PortType::In);
        bool areDependenciesMet =
            std::all_of(dependencies.begin(),
                        dependencies.end(),
                        [this](NodeId nodeId){
            Node* dependency = graph().findNode(nodeId);
            return dependency && pimpl->dataModel->nodeEvalState(dependency->uuid()) == NodeEvalState::Valid;
        });
        if (!areDependenciesMet)
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> dependencies not met!");
            continue;
        }

        auto const& inputPorts = node->ports(PortType::In);
        bool areInputsReady =
            std::all_of(inputPorts.begin(),
                        inputPorts.end(),
                        [](NodePort const& port){
            return port.isConnected() || port.optional;
        });
        if (!areInputsReady)
        {
            if (!isSilent())
                gtTrace().verbose()
                    << utils::logId(*this) << tr("-> inputs not met!");
            continue;
        }

        if ((size_t)node->nodeEvalMode() & IsExclusiveMask)
        {
            gtWarning()
                << utils::logId(*this) << tr("exclusive nod evaluation is not handled currently!");
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

