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
};

GraphExecutor::GraphExecutor(Graph& graph, GraphDataModel& dataModel) :
    QObject(&graph),
    pimpl(std::make_unique<Impl>(graph, dataModel))
{
    setObjectName(QStringLiteral("__executor"));
}

GraphExecutor::~GraphExecutor() = default;

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
    Queue next;

    // find "input" nodes
    ConnectionModel const& model = graph().connectionModel();
    utils::transform_if(
        model,
        // if operator
        [](ConnectionData<NodeId> const& data){
            return data.ports(PortType::In).empty();
        },
        std::back_inserter(next),
        // transformer
        [](ConnectionData<NodeId> const& data){
            return data.node;
        }
    );

    return Future{};
}

