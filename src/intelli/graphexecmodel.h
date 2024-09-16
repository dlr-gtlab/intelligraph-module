/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHEXECMODEL_H
#define GT_INTELLI_GRAPHEXECMODEL_H

#include <intelli/graph.h>
#include <intelli/nodedatainterface.h>

namespace intelli
{

class Connection;
class Graph;
class Node;

class ExecFuture;

/**
 * @brief The GraphExecutionModel class.
 * Manages the evaluation chain of a directed acyclic graph.
 */
class GT_INTELLI_EXPORT GraphExecutionModel : public QObject,
                                              public NodeDataInterface
{
    Q_OBJECT

    static constexpr size_t PRE_ALLOC = 20;

public:

    GraphExecutionModel(Graph& graph);
    ~GraphExecutionModel();

    static GraphExecutionModel* accessExecModel(Graph& graph);
    static GraphExecutionModel const* accessExecModel(Graph const& graph);

    Graph& graph();
    Graph const& graph() const;

    /**
     * @brief Resets the model, including all data and evaluation states.
     */
    void reset();

    GT_NO_DISCARD
    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const;

    /**
     * @brief Returns whether the root graph is currently beeing evaluated
     * @return Whether the root graph is beeing evaluated.
     */
    GT_NO_DISCARD
    bool isGraphEvaluated() const;

    /**
     * @brief Returns whether the graph is currently beeing evaluated
     * @param graph Graph
     * @return Whether the graph is beeing evaluated.
     */
    GT_NO_DISCARD
    bool isGraphEvaluated(Graph const& graph) const;

    /**
     * @brief Returns whether the node is currently evaluating
     * @param nodeUuid Node's uuid
     * @return Whether the node is evaluating. If node is not found, false is
     * returned.
     */
    GT_NO_DISCARD
    bool isNodeEvaluated(NodeUuid const& nodeUuid) const;

    /**
     * @brief Returns whether the model is currently evaluating any node.
     * @return Whether the model is currently evaluating a node.
     */
    GT_NO_DISCARD
    bool isEvaluating() const;

    GT_NO_DISCARD
    bool isAutoEvaluatingGraph() const;
    GT_NO_DISCARD
    bool isAutoEvaluatingGraph(Graph const& graph) const;

    bool autoEvaluateGraph();
    bool autoEvaluateGraph(Graph& graph);

    GT_NO_DISCARD
    ExecFuture evaluateGraph();
    GT_NO_DISCARD
    ExecFuture evaluateGraph(Graph& graph);
    GT_NO_DISCARD
    ExecFuture evaluateNode(NodeUuid const& nodeUuid);

    void stopAutoEvaluatingGraph();
    void stopAutoEvaluatingGraph(Graph& graph);

    bool invalidateNode(NodeUuid const& nodeUuid);
    bool invalidateNodeOutputs(NodeUuid const& nodeUuid);

    [[deprecated]] NodeDataSet nodeData(NodeId nodeId, PortId portId) const;
    [[deprecated]] bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data);

    NodeDataSet nodeData(Graph const& graph, NodeId nodeId, PortId portId) const;
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx) const;
    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override;

    bool setNodeData(Graph const& graph, NodeId nodeId, PortId portId, NodeDataSet data);
    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override;
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx, NodeDataSet data);
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override;

    /**
     * @brief Gives access to the internal exec model used to manage the
     * execution states and data of all nodes.
     * @return Execution data
     */
    inline auto const& data() const { return m_data; }

protected:

    void setNodeEvaluationFailed(NodeUuid const& nodeUuid) override;

    void nodeEvaluationStarted(NodeUuid const& nodeUuid) override;
    void nodeEvaluationFinished(NodeUuid const& nodeUuid) override;

signals:

    /**
     * @brief Emitted once a node has been evaluated. This signal
     * is always emitted in the next event loop cycle.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluated(QString const& nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once a node has failed to evaluate. This signal
     * may be emitted in the next event loop cycle.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluationFailed(QString const& nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once a node's eval state has changed.
     * @param nodeUuid Uuid of the affected node
     */
    void nodeEvalStateChanged(QString const& nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once an internal error has occured, that may result due to
     * a node not being accessible anymore (e.g. because it has been deleted).
     */
    void internalError(QPrivateSignal);

    void graphStalled(QPrivateSignal);

    void wakeup(QPrivateSignal);

private:

    // helper struct to "hide" implementation details
    struct Impl;

    /// assoicated graph
    QPointer<Graph> m_graph;
    /// data model for all nodes and their ports
    GraphDataModel m_data;
    /// nodes that should be evaluated
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_targetNodes;
    /// nodes that are pending
    std::vector<NodeUuid> m_pendingNodes;
    /// nodes that are ready and waiting for evaluation
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_queuedNodes;
    /// nodes that are currently evaluating
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_evaluatingNodes;

    /// graphs that should be auto evaluated
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_autoEvaluatingGraphs;
    /// indicator if the exec model is currently beeing modified and thus
    /// should halt execution
    int m_modificationCount = 0;

    void beginReset();

    void endReset();

    /**
     * @brief Will tell the model that new nodes and connections are about to be
     * inserted and pause the auto evaluation to avoid redundand evaluation of
     * nodes. Should be called when bulk inserting multiple objects.
     * Once `endModification` is called and the internal ref count reaches zero,
     * the model resumes evaluation.
     */
    void beginModification();

    void endModification();

    /**
     * @brief Whether this model is currently undergoing modification and
     * execution is halted
     * @return Is being modified
     */
    bool isBeingModified() const;

    /**
     * @brief Setups connections between the given graph and this exec model.
     * Required for updates to nodes and connections
     * @param graph Grap to subscribe to
     */
    void setupConnections(Graph& graph);

private slots:

    /**
     * @brief Method is directly invoked once a node has been evaluated.
     * @param nodeUuid Uuid of the node that was evaluated
     */
    void onNodeEvaluated(NodeUuid const& nodeUuid);

    void onNodeAppended(Node* node);

    void onNodeDeleted(Graph* graph, NodeId nodeId);

    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    void onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    void onConnectionAppended(ConnectionUuid conUuid);

    void onConnectionDeleted(ConnectionUuid conUuid);

    void onGraphDeleted();

    void onBeginGraphModification();

    void onEndGraphModification();
};

/// Outputs internal data of the graph exectuion model
GT_INTELLI_EXPORT void debug(GraphExecutionModel const& model);

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECMODEL_H
