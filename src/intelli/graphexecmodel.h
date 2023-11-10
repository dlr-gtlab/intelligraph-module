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

#include <gt_finally.h>
#include <gt_platform.h>

namespace intelli
{

class Connection;
class Graph;
class Node;

/**
 * @brief The DummyDataModel class.
 * Helper method to set and access node data of a single node
 */
class DummyDataModel : public NodeDataInterface
{
public:

    DummyDataModel(Node& node);

    dm::NodeDataSet nodeData(PortId portId, dm::NodeDataSet data);
    NodeDataPtrList nodeData(PortType type) const;

    bool setNodeData(PortId portId, dm::NodeDataSet data);
    bool setNodeData(PortType type, NodeDataPtrList const& data);

private:

    dm::NodeDataSet nodeData(NodeId nodeId, PortId portId) const override;
    bool setNodeData(NodeId nodeId, PortId portId, dm::NodeDataSet data) override;

    Node* m_node = nullptr;
    dm::Entry m_data;
};

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

    struct EndModificationFunctor
    {
        inline void operator()() const noexcept
        {
            if(model) model->endModification();
        }
        GraphExecutionModel* model{};
    };

    using Modification = gt::Finally<EndModificationFunctor>;

    GraphExecutionModel(Graph& graph, Mode mode = ActiveModel);
    ~GraphExecutionModel();

    Graph& graph();
    Graph const& graph() const;

    void makeActive();
    Mode mode() const;

    void reset();

    /**
     * @brief Static wrapper around `beginModification`. Will check if model is
     * a valid object and start the modification.
     * @param model
     * @return Scoped object which tells the model to begin reevaluating the
     * graph.
     */
    GT_NO_DISCARD
    static Modification modify(GraphExecutionModel* model);

    /**
     * @brief Will tell the model that new nodes and connections are about to be
     * inserted and pause the auto evaluation to avoid redundand evaluation of
     * nodes. Should be called when bulk inserting multiple objects.
     * Once the helper object is destroyed the model resumes evaluation.
     * @return Scoped object which tells the model to begin reevaluating the
     * graph.
     */
    GT_NO_DISCARD
    Modification beginModification();

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

    dm::NodeDataSet nodeData(NodeId nodeId, PortId portId) const override;
    dm::NodeDataSet nodeData(NodeId nodeId, PortType type, PortIndex portIdx) const;
    NodeDataPtrList nodeData(NodeId nodeId, PortType type) const;

    bool setNodeData(NodeId nodeId, PortId portId, dm::NodeDataSet data) override;
    bool setNodeData(NodeId nodeId, PortType type, PortIndex idx, dm::NodeDataSet data);
    bool setNodeData(NodeId nodeId, PortType type, NodeDataPtrList const& data);

    void debug() const;

signals:

    void nodeEvaluated(NodeId nodeId);

    void graphEvaluated();

    void internalError();

    void graphStalled();

private:

    struct Impl; // helper struct to "hide" implementation details

    dm::DataModel m_data;

    QPointer<Graph> m_graph;

    Mode m_mode;

    QVector<NodeId> m_targetNodes;

    QVector<NodeId> m_pendingNodes;

    QVector<NodeId> m_queuedNodes;

    QVector<QPointer<Node>> m_evaluatingNodes;

    // evaluation attributes

    bool m_autoEvaluate = false;

    bool m_isInserting = false;

    void beginReset();

    void endReset();

    void endModification();

    bool invalidatePort(NodeId nodeId, dm::PortEntry& port);

    bool autoEvaluateNode(NodeId nodeId);

    bool evaluateNodeDependencies(NodeId nodeId);

    /**
     * @brief Attempts to queue the node. Can only queue nodes that are ready
     * for evaluation (i.e. their inputs are met and are not already evaluating)
     * @param nodeId Node to queue
     * @return Success if node could be queued
     */
    bool queueNodeForEvaluation(NodeId nodeId);

    bool evaluateNextInQueue();

    void rescheduleTargetNodes();

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onNodeEvaluated();
};

class FutureGraphEvaluated
{
    friend class GraphExecutionModel;
public:

    FutureGraphEvaluated() = default;

    GT_INTELLI_EXPORT
    bool wait(std::chrono::milliseconds timeout = max_timeout);

    bool detach() { return hasStarted(); }

    bool hasStarted() const { return m_model; }

private:

    explicit FutureGraphEvaluated(GraphExecutionModel* model) :
        m_model(model)
    {}

    GraphExecutionModel* m_model;
};

class FutureNodeEvaluated
{
    friend class GraphExecutionModel;
public:

    FutureNodeEvaluated() = default;

    GT_INTELLI_EXPORT
    bool wait(std::chrono::milliseconds timeout = max_timeout);

    GT_INTELLI_EXPORT
    dm::NodeDataSet get(PortId port, std::chrono::milliseconds timeout = max_timeout);

    GT_INTELLI_EXPORT
    dm::NodeDataSet get(PortType type, PortIndex idx, std::chrono::milliseconds timeout = max_timeout);

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
