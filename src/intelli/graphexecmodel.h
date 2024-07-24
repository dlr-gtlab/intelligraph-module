/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_GRAPHEXECMODEL_H
#define GT_INTELLI_GRAPHEXECMODEL_H

#include <intelli/graph.h>
#include <intelli/nodedatamodel.h>

namespace intelli
{

class Connection;
class Graph;
class Node;

class FutureEvaluated;

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

    static GraphExecutionModel* accessExecModel(Graph& graph);
    static GraphExecutionModel const* accessExecModel(Graph const& graph);

    Graph& graph();
    Graph const& graph() const;

    void reset();

    GT_NO_DISCARD
    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const;

    GT_NO_DISCARD
    bool isGraphEvaluated() const;
    GT_NO_DISCARD
    bool isGraphEvaluated(Graph const& graph) const;
    GT_NO_DISCARD
    bool isNodeEvaluated(NodeUuid const& nodeUuid) const;

    GT_NO_DISCARD
    bool isAutoEvaluatingGraph() const;
    GT_NO_DISCARD
    bool isAutoEvaluatingGraph(Graph const& graph) const;
    GT_NO_DISCARD
    bool isAutoEvaluatingNode(NodeUuid const& nodeUuid) const;

    GT_NO_DISCARD
    FutureEvaluated autoEvaluateGraph();
    GT_NO_DISCARD
    FutureEvaluated autoEvaluateGraph(Graph& graph);
    GT_NO_DISCARD
    FutureEvaluated autoEvaluateNode(NodeUuid const& nodeUuid);

    GT_NO_DISCARD
    FutureEvaluated evaluateGraph();
    GT_NO_DISCARD
    FutureEvaluated evaluateGraph(Graph& graph);
    GT_NO_DISCARD
    FutureEvaluated evaluateNode(NodeUuid const& nodeUuid);

    void stopAutoEvaluatingGraph();
    void stopAutoEvaluatingGraph(Graph& graph);
    void stopAutoEvaluatingNode(NodeUuid const& nodeUuid);

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

private:

    // helper struct to "hide" implementation details
    struct Impl;

    enum class NodeEvaluationType
    {
        SingleShot = 0,
        KeepEvaluated
    };

    struct TargetNode
    {
        NodeUuid nodeUuid;
        NodeEvaluationType evalType;
    };

    /// assoicated graph
    QPointer<Graph> m_graph;

    GlobalConnectionGraph m_model;

    /// data model for all nodes and their ports
    GraphDataModel m_data;
    /// nodes that should be evaluated
    QVarLengthArray<TargetNode, PRE_ALLOC> m_targetNodes;
//    QVarLengthArray<NodeUuid, PRE_ALLOC> m_pendingNodes;
    /// nodes that are ready and waiting for evaluation
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_queuedNodes;
    /// indicator if the exec model is currently beeing modified and thus
    /// should halt execution
    int m_modificationCount = 0;
    /// whether to auto evaluate the root graph
    bool m_autoEvaluateRootGraph = false;

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

    void setupConnections(Graph& graph);

private slots:

    /**
     * @brief Method is directly invoked once a node has been evaluated.
     * The node evaluated is given by `sender()`.
     */
    void onNodeEvaluatedHelper();

    /**
     * @brief Method is invoked in the next event loop cycle once the node
     * associated with `nodeUuid` has been evaluated.
     * @param nodeUuid Node that has been evaluated
     */
    void onNodeEvaluated(QString const& nodeUuid);

    void onNodeAppended(Node* node);

    void onNodeDeleted(Graph* graph, NodeId nodeId);

    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    void onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    void onBeginGraphModification();

    void onEndGraphModification();

    void onConnectionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);
};

/// Outputs internal data of the graph exectuion model
GT_INTELLI_EXPORT void debug(GraphExecutionModel const& model);

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECMODEL_H
