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
#include <intelli/graphdatamodel.h>
#include <intelli/nodedatainterface.h>

#include <QPointer>

namespace intelli
{

class Graph;
class Node;

/**
 * @brief The GraphExecutionModel class.
 * Manages the evaluation chain of a directed acyclic graph.
 *
 * By default all nodes are invalidated (=outdated) and require an evaluation.
 * No node or graph is auto matically evalauted by default.
 *
 * One exec model is required to evaluate a graph hierarchy. To access node
 * data the uuid is used instead of the node's id.
 */
class GT_INTELLI_EXPORT GraphExecutionModel : public NodeDataInterface
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

    /**
     * @brief Creates and appends a execution model to the graph hierarchy of
     * `graph` and returns a pointer to the newely created exec model. The
     * ownership is taken care of. If a exec model already exists for `graph`,
     * it is returned instead.
     * @param graph Graph to create and append exec model for.
     * @return Pointer to current exec model.
     */
    static GraphExecutionModel* make(Graph& graph);

    /**
     * @brief Provides access to the current graph
     * @return The associated graph of the model
     */
    GT_NO_DISCARD
    Graph& graph();
    GT_NO_DISCARD
    Graph const& graph() const;

    /**
     * @brief Resets the model, including all data and evaluation states.
     */
    void reset();

    /**
     * @brief Resets all nodes that were previously marked for evaluation using
     * `evaluateGraph` or `evaluateNode`.
     */
    void resetTargetNodes();

    /**
     * @brief Returns the node evaluation state of the given node
     * @param nodeUuid Node Uuid of the requested node.
     * @return Node eval state
     */
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

    /**
     * @brief Starts the auto evaluation of the associated graph of this model.
     * Use this method to keep the nodes of the associated graph up-to-date.
     * @return Whether the auto evaluation could be started successfully.
     */
    bool autoEvaluateGraph();
    /**
     * @brief Starts the auto evaluation of the given graph node.
     * Use this method to keep the nodes of the given graph up-to-date.
     * @return Whether the auto evaluation could be started successfully.
     */
    bool autoEvaluateGraph(Graph& graph);

    /**
     * @brief Starts the evaluation of all nodes that belong the graph
     * associated with this exec model. Any inactive node is also evalauted.
     * Use this method to evaluate the nodes of the associated graph exactly
     * once.
     * @return Future object
     */
    GT_NO_DISCARD
    ExecFuture evaluateGraph();
    /**
     * @brief Starts the evaluation of all nodes that belong the given graph.
     * Any inactive node is also evalauted. Use this method to evaluate the
     * nodes of the associated graph exactly once.
     * @return Future object
     */
    GT_NO_DISCARD
    ExecFuture evaluateGraph(Graph& graph);
    /**
     * @brief Starts the evaluation of the given node including all
     * dependencies. Any inactive node is also evalauted. Use this method to
     * evaluate the nodes of the associated graph exactly once.
     * @return Future object
     */
    GT_NO_DISCARD
    ExecFuture evaluateNode(NodeUuid const& nodeUuid);

    /**
     * @brief Stops the auto evaluation of the graph that is associated with
     * this exec model.
     */
    void stopAutoEvaluatingGraph();
    /**
     * @brief Stops the auto evaluation of the given graph.
     */
    void stopAutoEvaluatingGraph(Graph& graph);

    /**
     * @brief Invalidates the specified node. All output data is invalidated
     * but not cleared. The node must be reevaluated to update the node once
     * more.
     * @param nodeUuid Uuid of the node to invalidate.
     * @return success
     */
    bool invalidateNode(NodeUuid const& nodeUuid);

    GT_NO_DISCARD
    [[deprecated]] NodeDataSet nodeData(NodeId nodeId, PortId portId) const;
    [[deprecated]] bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data);

    /**
     * @brief Node-id based overload to access the data of a node.
     * @param graph Graph that is the direct parent of the spcified node
     * @param nodeId Node id
     * @param portId Desired port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(Graph const& graph, NodeId nodeId, PortId portId) const;
    /**
     * @brief Returns the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param portId Desired port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;
    /**
     * @brief Returns the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param type Whether the port is an input or output port
     * @param portIdx Index of the port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx) const;
    /**
     * @brief Returns the node data of the given node as a list. Each entry is
     * associtated with the given port id.
     * @param nodeUuid Node's uuid
     * @param type Whether to access the input or output ports
     * @return List of node datasets (may be null)
     */
    GT_NO_DISCARD
    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override;

    /**
     * @brief Node-id based overload to set the data of a node.
     * @param graphGraph that is the direct parent of the spcified node
     * @param nodeId Node id
     * @param portId Desired port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(Graph const& graph, NodeId nodeId, PortId portId, NodeDataSet data);
    /**
     * @brief Sets the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param portId Desired port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override;
    /**
     * @brief Sets the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param type Whether the port is an input or output port
     * @param portIdx Index of the port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx, NodeDataSet data);
    /**
     * @brief Applies a list of node data to the given node.
     * @param nodeUuid Node's uuid
     * @param type Whether to access the input or output ports
     * @param data Data list to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override;

    /**
     * @brief Gives access to the internal exec model used to manage the
     * execution states and data of all nodes.
     * @return Execution data
     */
    GT_NO_DISCARD
    inline GraphDataModel const& data() const;

    /**
     * @brief Returns the user variables object if any exists.
     * @return User variables object (may be null)
     */
    GT_NO_DISCARD
    GraphUserVariables const* userVariables() const override;

    /**
     * @brief Returns the scope object used for evalauation. The scope object
     * is intended to to be used for interfacing with the datamodel.
     * By default the scope object s the current project, but it may be set to
     * a sub datatree or an external tree instead.
     * @return Scope object (may be null).
     */
    GtObject* scope() override;

    /**
     * @brief Sets the scope object that should be used for evalauation.
     * @param Scope object
     */
    void setScope(GtObject* scope);

protected:

    /**
     * @brief Called once a node fails its evaluation. Must be called while
     * the node is still evalauting
     * @param nodeUuid Node's Uuid
     */
    void setNodeEvaluationFailed(NodeUuid const& nodeUuid) override;

    /**
     * @brief Called once when a node starts its evalaution. Must be followed
     * by `nodeEvaluationFinished`.
     * @param nodeUuid Node's Uuid
     */
    void nodeEvaluationStarted(NodeUuid const& nodeUuid) override;

    /**
     * @brief Called once when a node finishes its evalaution. Must follow
     * `nodeEvaluationStarted`.
     * @param nodeUuid Node's Uuid
     */
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

    /**
     * @brief Emitted if a graph has stalled evaluation.
     */
    void graphStalled(QPrivateSignal);

    /**
     * @brief Attempts to trigger the evaluation of nodes that are queued and
     * could not be evalauted due to cross dependencies to other exec models
     */
    void wakeup(QPrivateSignal);

    /**
     * @brief Emiited once the auto evalaution state of `graph` has changed.
     * @param graph Graph that has changed its auto evaluation state.
     */
    void autoEvaluationChanged(Graph* graph);

private:

    // helper struct to "hide" implementation details
    struct Impl;
    std::unique_ptr<Impl> pimpl;

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

    /// Called once a node finishes its evalaution. Triggers the evaluation of
    /// successor nodes.
    void onNodeEvaluated(NodeUuid const& nodeUuid);

    /// Updates the model if a node was appended
    void onNodeAppended(Node* node);

    /// Updates the model if a node was deleted
    void onNodeDeleted(Graph* graph, NodeId nodeId);

    /// Updates the model if a port was inserted
    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    /// Updates the model if a port is about to be deleted
    void onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    /// Updates the model if a connection was appended
    void onConnectionAppended(ConnectionUuid conUuid);

    /// Updates the model if a connection was deleted
    void onConnectionDeleted(ConnectionUuid conUuid);

    /// Updates the model if a graph was deleted
    void onGraphDeleted();

    /// Called if a graph is being modified
    void onBeginGraphModification();

    /// Called if a graph was modified
    void onEndGraphModification();
};

/// Outputs internal data of the graph exectuion model
GT_INTELLI_EXPORT void debug(GraphExecutionModel const& model);

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECMODEL_H
