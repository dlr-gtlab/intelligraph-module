/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/graphexecutor.h"

#include <intelli/graph.h>

#include <QPointer>

using namespace intelli;

struct GraphExecutor::Impl
{
    explicit Impl(Graph& g) : graph(&g) {}

    /// associated graph
    QPointer<Graph> graph;
};

GraphExecutor::GraphExecutor(Graph& graph) :
    QObject(&graph),
    pimpl(std::make_unique<Impl>(graph))
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

