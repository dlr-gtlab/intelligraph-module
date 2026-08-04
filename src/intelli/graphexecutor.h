/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#ifndef GT_INTELLI_GRAPHEXECUTOR_H
#define GT_INTELLI_GRAPHEXECUTOR_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QObject>

namespace intelli
{

class Graph;
class GraphExecutor : public QObject
{
    Q_OBJECT

public:

    GraphExecutor(Graph& graph);
    ~GraphExecutor();

    Graph& graph();
    Graph const& graph() const;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECUTOR_H
