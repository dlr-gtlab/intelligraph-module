/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include <intelli/graph.h>
#include <intelli/nodedatainterface.h>

namespace intelli
{

class Connection;
class Graph;
class Node;
class FutureGraphEvaluated;
class FutureNodeEvaluated;

/**
 * @brief The GraphExecutionModel class.
 * Manages the evaluation chain of a directed acyclic graph.
 */
class GT_INTELLI_EXPORT GraphExecutionModel : public QObject,
                                              public NodeDataInterface
{
    Q_OBJECT

public:

    enum Mode
    {
        DummyModel = 1,
        ActiveModel = 2
    };

    GraphExecutionModel(Graph& graph, Mode mode = ActiveModel);
    ~GraphExecutionModel();

    Graph& graph();
    Graph const& graph() const;

    void makeActive();
    Mode mode() const;

    void reset();

    NodeEvalState nodeEvalState(NodeId nodeId) const;

    /**
     * @brief Returns whether the underlying graph has been evaluated completly,
     * i.e. all nodes have been evaluated once and their data is valid.
     * @return Returns whether the graph has been evaluated.
     */
    bool isEvaluated() const;

    /**
     * @brief Returns whether the node has been evaluated and if its data is
     * valid.
     * @return Returns whether the graph has been evaluated.
     */
    bool isNodeEvaluated(NodeId nodeId) const;

    /**
     * @brief Whether the model is auto-evaluating the graph. When
     * auto-evaluating changes to the input/output data of a node will trigger
     * dependent nodes automatically.
     * @return Is auto-evaluating
     */
    bool isAutoEvaluating() const;

    /*
     * @brief Tells the model to disable the auto evaluation (if it has been
     * enabled)
     */
    void disableAutoEvaluation();

    /**
     * @brief Tells the model to auto evaluate the graph. When auto-evaluatiing
     * changes to the input/output data of a node will trigger dependent nodes
     * automatically.
     * @return Future like object that can be used to pause until the evaluation
     * finishes. The model must be kept alive for the life time of this object.
     */
    GT_NO_DISCARD
    FutureGraphEvaluated autoEvaluate();

    /**
     * @brief Tells the model to evaluate the graph once. This will not keep the
     * model evaluating if inputs/outputs change.
     * @return Future like object that can be used to pause until the evaluation
     * finishes. The model must be kept alive for the life time of this object.
     */
    GT_NO_DISCARD
    FutureGraphEvaluated evaluateGraph();

    /**
     * @brief Tells the model to evaluate a single node. Multiple calls to this
     * function with different nodes are allowed and will be handled accordinly
     * @param nodeId
     * @return Future like object that can be used to pause until the evaluation
     * finishes. The model must be kept alive for the life time of this object.
     */
    GT_NO_DISCARD
    FutureNodeEvaluated evaluateNode(NodeId nodeId);

    bool invalidateOutPorts(NodeId nodeId);
    bool invalidatePort(NodeId nodeId, PortId portId);
    bool invalidateNode(NodeId nodeId);

    NodeDataSet nodeData(NodeId nodeId, PortId portId) const override;
    NodeDataSet nodeData(NodeId nodeId, PortType type, PortIndex portIdx) const;
    NodeDataPtrList nodeData(NodeId nodeId, PortType type) const;

    bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data) override;
    bool setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeDataSet data);
    bool setNodeData(NodeId nodeId, PortType type, NodeDataPtrList const& data);

    void debug(NodeId nodeId = invalid<NodeId>()) const;

signals:

    void nodeEvaluated(NodeId nodeId);

    void graphEvaluated();

    void internalError();

    void graphStalled();

    void nodeEvalStateChanged(NodeId nodeId);

private:

    struct Impl; // helper struct to "hide" implementation details

    GraphData m_data;

    QPointer<Graph> m_graph;

    Mode m_mode;

    QVector<NodeId> m_targetNodes;

    QVector<NodeId> m_pendingNodes;

    QVector<NodeId> m_queuedNodes;

    QVector<QPointer<Node>> m_evaluatingNodes;

    // evaluation properties & flags
    int m_modificationCount = 0;
    bool m_autoEvaluate = false;
    bool m_evaluatingPendingNodes = false;

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

    bool invalidatePortEntry(NodeId nodeId, graph_data::PortEntry& port);

    bool autoEvaluateNode(NodeId nodeId);

    bool evaluateNodeDependencies(NodeId nodeId, bool reevaluate = false);

    /**
     * @brief Attempts to queue the node. Can only queue nodes that are ready
     * for evaluation (i.e. their inputs are met and are not already evaluating)
     * @param nodeId Node to queue
     * @return Success if node could be queued
     */
    bool queueNodeForEvaluation(NodeId nodeId);

    bool evaluateNextInQueue();

    void rescheduleTargetNodes();

    QVector<NodeId> findRootNodes(QList<Node*> const& nodes) const;

    QVector<NodeId> findLeafNodes(QList<Node*> const& nodes) const;

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    void onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    void onBeginGraphModification();

    void onEndGraphModification();

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onNodeEvaluated();
};

/**
 * @brief The FutureGraphEvaluated class.
 * Helper class to await evaluation of a complete graph
 */
class FutureGraphEvaluated
{
    friend class GraphExecutionModel;
public:

    FutureGraphEvaluated() = default;

    GT_INTELLI_EXPORT
        bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    bool detach() { return hasStarted(); }

    bool hasStarted() const { return m_model; }

private:

    explicit FutureGraphEvaluated(GraphExecutionModel* model) :
        m_model(model)
    {}

    GraphExecutionModel* m_model;
};

/**
 * @brief The FutureNodeEvaluated class.
 * Helper class to await evaluation of a node
 */
class FutureNodeEvaluated
{
    friend class GraphExecutionModel;
public:

    FutureNodeEvaluated() = default;

    GT_INTELLI_EXPORT
    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    GT_INTELLI_EXPORT
    NodeDataSet get(PortId port, std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    GT_INTELLI_EXPORT
    NodeDataSet get(PortType type, PortIndex idx, std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    bool detach() { return hasStarted(); }

    bool hasStarted() const { return m_model && m_targetNode != invalid<NodeId>(); }

private:

    explicit FutureNodeEvaluated(GraphExecutionModel* model, NodeId target) :
        m_model(model), m_targetNode(target)
    {}

    GraphExecutionModel* m_model{};
    NodeId m_targetNode{};
};

} // namespace intelli

#endif // GRAPHMODEL_H
