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
#include <intelli/nodedata.h>

namespace intelli
{

class Connection;
class Graph;
class Node;

enum class PortDataState
{
    /// Port data was outdata
    Outdated = 0,
    /// Port data is valid and up-to-date
    Valid,
};

namespace dm
{

struct PortEntry
{
    /// referenced port
    PortId id;
    /// port data state
    PortDataState state = PortDataState::Outdated;
    /// actual data at port
    NodeDataPtr   data  = nullptr;
};

/// helper struct representing node data and its validity state
struct NodeData
{
    NodeData(NodeDataPtr _data = {}) :
        data(std::move(_data)), state(PortDataState::Valid)
    {}
    template <typename T>
    NodeData(std::shared_ptr<T> _data) :
        data(std::move(_data)), state(PortDataState::Valid)
    {}
    NodeData(PortEntry const& port) :
        data(port.data), state(port.state)
    {}

    /// actual node data
    NodeDataPtr data;
    /// data state
    PortDataState state;

    operator NodeDataPtr&() & { return data; }
    operator NodeDataPtr() && { return std::move(data); }
    operator NodeDataPtr const&() const& { return data; }

    template <typename T>
    inline auto value() const noexcept { return qobject_pointer_cast<T const>(data);}
};

struct Entry
{
    /// in and out ports
    QVector<PortEntry> portsIn = {}, portsOut = {};

    GT_INTELLI_EXPORT bool isEvaluated(Node const& node) const;

    GT_INTELLI_EXPORT bool areInputsValid(Graph const& graph, NodeId nodeId) const;

    GT_INTELLI_EXPORT bool canEvaluate(Graph const& graph, Node const& node) const;
};

using DataModel = QHash<NodeId, Entry>;

} // namesace dm

/**
 * @brief The NodeDataInterface class.
 * Interface to access and set the data of a node port
 */
class NodeDataInterface
{
public:

    virtual ~NodeDataInterface() = default;

    virtual dm::NodeData nodeData(NodeId nodeId, PortId portId) const = 0;

    virtual bool setNodeData(NodeId nodeId, PortId portId, dm::NodeData data) = 0;

};

class DummyDataModel : public NodeDataInterface
{
    DummyDataModel(Node& node);

    NodeId nodeId;
    dm::Entry data;
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

    GraphExecutionModel(Graph& graph, Mode mode = ActiveModel);

    Mode mode() const;

    Graph& graph();
    Graph const& graph() const;

    void beginReset();
    void endReset();
    void reset();

    bool evaluated();

    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    bool waitForNode(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    bool autoEvaluate(bool enable = true);
    bool isAutoEvaluating() const;

    bool evaluateNode(NodeId nodeId);

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

    void invalidatePort(NodeId nodeId, dm::PortEntry& port);

    bool triggerNodeExecution(NodeId nodeId);

    bool triggerNode(NodeId nodeId, PortId portId = invalid<PortId>());

    void evaluateTargetNode(NodeId nodeId = invalid<NodeId>());

private slots:

    void appendNode(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onNodeEvaluated();
};

} // namespace intelli

#endif // GRAPHMODEL_H
