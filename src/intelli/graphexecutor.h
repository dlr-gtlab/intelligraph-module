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

using Future = bool;

class Graph;
class GraphDataModel;
class GT_INTELLI_EXPORT GraphExecutor : public QObject
{
    Q_OBJECT

public:

    GraphExecutor(Graph& graph, GraphDataModel& dataModel);
    ~GraphExecutor();

    Graph& graph();
    Graph const& graph() const;

    GraphDataModel& dataModel();
    GraphDataModel const& dataModel() const;

    /**
     * @brief Resets the model.
     */
    void reset();

    /**
     * @brief Starts the evaluation of all nodes that belong the graph
     * associated with this exec model. Any inactive node is also evalauted.
     * Use this method to evaluate the nodes of the associated graph exactly
     * once.
     * @return Future object
     */
    GT_NO_DISCARD
    Future evaluateAll();

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECUTOR_H
