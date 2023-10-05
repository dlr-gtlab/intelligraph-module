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

enum ExecMode
{
    Blocking,
    Async
};

/**
 * @brief The DummyDataModel class.
 * Helper method to set and access node data of a single node
 */
class DummyDataModel : public NodeDataInterface
{
public:

    DummyDataModel(Node& node);

    dm::NodeData nodeData(PortId portId, dm::NodeData data);
    NodeDataPtrList nodeData(PortType type) const;

    bool setNodeData(PortId portId, dm::NodeData data);
    bool setNodeData(PortType type, NodeDataPtrList const& data);

private:

    dm::NodeData nodeData(NodeId nodeId, PortId portId) const override;
    bool setNodeData(NodeId nodeId, PortId portId, dm::NodeData data) override;

    Node* m_node = nullptr;
    dm::Entry m_data;
};

/**
 * @brief The GraphExecutionModel class.
 * Manages the evaluation chain of a directed acyclic graph.
 */
class GT_INTELLI_EXPORT GraphExecutionModel : public QObject,
                                              public NodeDataInterface
{
    Q_OBJECT

public:

    enum Option
    {
        NoOption = 0,
        DoNotTrigger = 1,
    };

    enum Mode
    {
        DummyModel = 1,
        ActiveModel = 2
    };

    struct EndInsertionFunctor
    {
        inline void operator()() const noexcept
        {
            if(model) model->endInsertion();
        }
        GraphExecutionModel* model;
    };

    using Insertion = gt::Finally<EndInsertionFunctor>;

    GraphExecutionModel(Graph& graph, Mode mode = ActiveModel);
    ~GraphExecutionModel();

    Graph& graph();
    Graph const& graph() const;

    void makeActive();
    Mode mode() const;

    void reset();

    /**
     * @brief Will tell the model that new nodes and connections are about to be
     * inserted and pause the auto evaluation to avoid redundand evaluation of
     * nodes. Should be called when bulk inserting multiple objects.
     * Once the helper object is destroyed the model resumes evaluation.
     * @return Scoped object which tells the model to begin reevaluating the
     * graph.
     */
    GT_NO_DISCARD Insertion beginInsertion();

    bool evaluated();

    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    bool waitForNode(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    bool autoEvaluate(bool enable = true);
    bool isAutoEvaluating() const;

    struct Future
    {
        GraphExecutionModel* model;

        bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

        bool detach();

        bool then(std::function<void(int success)>);
    };


    GT_NO_DISCARD
    Future evaluateGraph();

    bool evaluateNode(NodeId nodeId, ExecMode mode = ExecMode::Blocking);

    bool invalidateOutPorts(NodeId nodeId);
    bool invalidatePort(NodeId nodeId, PortId portId);

    dm::NodeData nodeData(NodeId nodeId, PortId portId) const override;
    dm::NodeData nodeData(NodeId nodeId, PortType type, PortIndex portIdx) const;
    NodeDataPtrList nodeData(NodeId nodeId, PortType type) const;

    bool setNodeData(NodeId nodeId, PortId portId, dm::NodeData data) override;
    bool setNodeData(NodeId nodeId, PortId portId, dm::NodeData data, int option);
    bool setNodeData(NodeId nodeId, PortType type, PortIndex idx, dm::NodeData data, int option = Option::NoOption);
    bool setNodeData(NodeId nodeId, PortType type, NodeDataPtrList const& data, int option = Option::NoOption);

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

    // evaluation attributes

    NodeId m_targetNodeId = invalid<NodeId>();

    bool m_autoEvaluate = false;

    bool m_isInserting = false;

    void beginReset();

    void endReset();

    void endInsertion();

    void invalidatePort(NodeId nodeId, dm::PortEntry& port);

    bool autoEvaluateNode(NodeId nodeId);

    bool evaluateDependencies(NodeId nodeId);

    void evaluateTargetNode(NodeId nodeId = invalid<NodeId>());

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onNodeEvaluated();
};

} // namespace intelli

#endif // GRAPHMODEL_H
