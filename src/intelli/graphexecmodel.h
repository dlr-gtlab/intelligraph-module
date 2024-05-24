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

class FutureFGraphEvaluated;
class FutureNodeEvaluated;

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

    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid);

    bool autoEvaluateGraph();
    bool autoEvaluateNode(NodeUuid const& nodeUuid);

    FutureNodeEvaluated evaluateGraph();
    FutureNodeEvaluated evaluateNode(NodeUuid const& nodeUuid);

    void stopAutoEvaluatingGraph();
    void stopAutoEvaluatingNode(NodeUuid const& nodeUuid);

    bool isAutoEvaluatingNode(NodeUuid const& nodeUuid) const;
    bool isAutoEvaluatingGraph() const;

    bool invalidateNode(NodeUuid const& nodeUuid);
    bool invalidateNodeOutputs(NodeUuid const& nodeUuid);

    [[deprecated]] NodeDataSet nodeData(NodeId nodeId, PortId portId) const;
    [[deprecated]] bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data);

    NodeDataSet nodeData(Graph const& graph, NodeId nodeId, PortId portId) const;
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;
    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override;

    bool setNodeData(Graph const& graph, NodeId nodeId, PortId portId, NodeDataSet data);
    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override;
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override;

    /**
     * @brief Gives access to the internal exec model used to manage the
     * execution states and data of all nodes.
     * @return Execution data
     */
    inline auto const& data() const { return m_data; }

    auto& queue() const { return m_queuedNodes; }
    auto& targets() const { return m_targetNodes; }

signals:

    /**
     * @brief Emitted once a node has been evaluated. This signal
     * is always emitted in the next event loop cycle.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluated(QString nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once a node has failed to evaluate. This signal
     * may be emitted in the next event loop cycle.
     * @param nodeUuid Uuid of the evaluated node
     */
    void nodeEvaluationFailed(QString nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once a node's eval state has changed.
     * @param nodeUuid Uuid of the affected node
     */
    void nodeEvalStateChanged(QString nodeUuid, QPrivateSignal);

    /**
     * @brief Emitted once an internal error has occured, that may result due to
     * a node not being accessible anymore (e.g. because it has been deleted).
     */
    void internalError(QPrivateSignal);

    void graphStalled(QPrivateSignal);

private:

    struct Impl; // helper struct to "hide" implementation details

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
    /// data model for all nodes and their ports
    GraphDataModel m_data;
    /// nodes that should be evaluated
    QVarLengthArray<TargetNode, PRE_ALLOC> m_targetNodes;
    /// nodes that are scheduled but not ready yet
//    QVarLengthArray<NodeUuid, PRE_ALLOC> m_pendingNodes;
    /// nodes that are ready and waiting for evaluation
    QVarLengthArray<NodeUuid, PRE_ALLOC> m_queuedNodes;
    /// indicator if the exec model is currently beeing modified and thus
    /// should halt execution
    int m_modificationCount = 0;
    int m_indent = 0;
    /// whether to auto evaluate the associated graph
    bool m_autoEvaluateGraph = false;

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
    void onNodeEvaluatedAsync(QString nodeUuid);

    void onNodeAppended(Node* node);

    void onNodeDeleted(Graph* graph, NodeId nodeId);

    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    void onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    void onBeginGraphModification();

    void onEndGraphModification();

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);
};

/// Outputs internal data of the graph exectuion model
GT_INTELLI_EXPORT void debug(GraphExecutionModel const& model);

class FutureNodeEvaluated
{
    friend class GraphExecutionModel;

    static constexpr size_t PRE_ALLOC = 5;

public:

    GT_NO_DISCARD
    GT_INTELLI_EXPORT
    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    GT_INTELLI_EXPORT
    bool detach();

    /**
     * @brief Join with other futures. Canbe used to wait for multiple,
     * separatly triggered nodes.
     * @param other Other future to join with
     * @return returns a new future
     */
    GT_NO_DISCARD
    GT_INTELLI_EXPORT
    FutureNodeEvaluated& join(FutureNodeEvaluated const& other);

private:

    struct TargetNode
    {
        NodeUuid uuid;
        NodeEvalState evalState;
    };

    /// pointer to the source execution model
    QPointer<GraphExecutionModel> m_model;
    /// targets to watch, contains only invalid or still evaluating targets
    QVarLengthArray<TargetNode, PRE_ALLOC> m_targets;

    /**
     * @brief Constructs a future object from the model and registers the node
     * @param model Source exection model
     * @param nodeUuid Node to register
     * @param evalState Current node eval state
     */
    FutureNodeEvaluated(GraphExecutionModel& model,
                        NodeUuid nodeUuid,
                        NodeEvalState evalState);

    /**
     * @brief Constructs an empty future object from the model
     * @param model Source exection model
     */
    explicit FutureNodeEvaluated(GraphExecutionModel& model);

    /**
     * @brief Registers the node to this future.
     * @param nodeUuid Node to register
     * @param evalState Current node eval state
     * @return this
     */
    FutureNodeEvaluated& append(NodeUuid nodeUuid, NodeEvalState evalState);

    bool isFinished() const;

    bool containsFailedNodes() const;

    void updateTargets();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECMODEL_H
