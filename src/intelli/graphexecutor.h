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

class Node;
class Graph;
class GraphDataModel;
class GT_INTELLI_EXPORT GraphExecutor : public QObject
{
    Q_OBJECT

public:

    GraphExecutor(Graph& graph, GraphDataModel& dataModel);
    ~GraphExecutor();

    bool isSilent() const;
    void setSilent(bool value);

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
    Future evaluateGraph();

    GT_NO_DISCARD
    Future evaluateNode(NodeId nodeId);

    void autoEvaluate(bool enable = true);

    [[deprecated("use `autoEvaluate(false)` instead")]]
    void stopAutoEvaluatingGraph() { autoEvaluate(false); }

    [[deprecated("use `autoEvaluate(true)` instead")]]
    void autoEvaluateGraph() { autoEvaluate(true); }

    GT_NO_DISCARD
    bool isAutoEvaluating() const;

    GT_NO_DISCARD
    [[deprecated("use `isAutoEvaluating` instead")]]
    bool isAutoEvaluatingGraph() const { return isAutoEvaluating(); }

signals:

    void allNodesEvaluated();

    void targetNodesEvaluated();

    void autoEvaluationChanged();

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    void queuePending();

    void evaluateQueue();

    void onNodeEvaluationStarted(QString const& nodeUuid);

    void onNodeEvaluationFinished(QString const& nodeUuid);

    void onNodeEvaluated(NodeUuid const& nodeUuid);

    void setupConnections(Graph& graph);

    void onNodeAppended(Node* node);

    void onConnectionAppended(ConnectionId conId);

    void onConnectionDeleted(ConnectionId conId);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECUTOR_H
