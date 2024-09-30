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

#include <intelli/future.h>
#include <intelli/nodedatainterface.h>

#include <QPointer>

namespace intelli
{

class Connection;
class Graph;
class Node;

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

    GT_NO_DISCARD
    static GraphExecutionModel* accessExecModel(Graph& graph);
    GT_NO_DISCARD
    static GraphExecutionModel const* accessExecModel(Graph const& graph);

    GT_NO_DISCARD
    Graph& graph();
    GT_NO_DISCARD
    Graph const& graph() const;

    /**
     * @brief Resets the model, including all data and evaluation states.
     */
    void reset();

    GT_NO_DISCARD
    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const override;

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

    /**
     * @brief Returns whether the associated graph is marked for auto
     * evaluation. Graphs that are auto evaluating will attempt to keep all
     * nodes evaluated and valid.
     * @return Whether the associated graph is marked for auto evaluation
     */
    GT_NO_DISCARD
    bool isAutoEvaluatingGraph() const;
    /**
     * @brief Returns whether the given graph is marked for auto
     * evaluation. Graphs that are auto evaluating will attempt to keep all
     * nodes evaluated and valid.
     * @return Whether the associated graph is marked for auto evaluation
     */
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

    GT_NO_DISCARD
    [[deprecated]] NodeDataSet nodeData(NodeId nodeId, PortId portId) const;
    [[deprecated]] bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data);

    GT_NO_DISCARD
    NodeDataSet nodeData(Graph const& graph, NodeId nodeId, PortId portId) const;
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx) const;
    GT_NO_DISCARD
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
    GT_NO_DISCARD
    inline auto const& data() const { return m_data; }

protected:

    void setNodeEvaluationFailed(NodeUuid const& nodeUuid) override;

    void nodeEvaluationStarted(NodeUuid const& nodeUuid) override;

    void nodeEvaluationFinished(NodeUuid const& nodeUuid) override;

signals:

    /**
     * @brief Emitted once a node has been evaluated.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluated(QString const& nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once a node has failed to evaluate.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluationFailed(QString const& nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once an internal error has occured, that may result due to
     * a node unexpectedly not being accessible anymore.
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
    std::vector<NodeUuid> m_targetNodes;
    /// nodes that should be queued and executed at some point to evaluate
    /// all target nodes
    std::vector<NodeUuid> m_pendingNodes;
    /// nodes that should be considered for auto evaluation
    std::set<NodeUuid> m_autoEvaluatingNodes;
    /// nodes that are ready and waiting for evaluation
    std::vector<NodeUuid> m_queuedNodes;
    /// nodes that are currently evaluating
    std::vector<NodeUuid> m_evaluatingNodes;
    /// graphs that should be auto evaluated
    std::vector<NodeUuid> m_autoEvaluatingGraphs;
    /// indicator if the exec model is currently beeing modified and thus
    /// should halt execution
    int m_modificationCount = 0;

    void beginReset();

    void endReset();

    /**
     * @brief Will tell the model that new nodes and connections are about to be
     * inserted and pause the auto evaluation to avoid unnecessary updates and
     * evaluations of nodes. Should be called when bulk inserting/deleting nodes
     * and connections. Once `endModification` is called and the internal ref
     * count reaches zero, the model resumes evaluation.
     */
    void beginModification();

    /**
     * @brief Resumes evaluation of nodes if the internal ref count reaches zero.
     */
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
