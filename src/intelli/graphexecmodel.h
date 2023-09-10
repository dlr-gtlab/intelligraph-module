/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include <intelli/node.h>
#include <intelli/nodedata.h>

#include <QtNodes/AbstractGraphModel>

#include <QPointer>

namespace intelli
{

class Connection;
class Graph;

enum class PortDataState
{
    Outdated = 0,
    Valid,
};

class GT_INTELLI_EXPORT GraphExecutionModel : public QObject
{
    Q_OBJECT

public:

    enum class NodeEvalState
    {
        Evaluated = 0,
        Evaluating,
        ManualEvaluation
    };

    enum Option
    {
        NoOption = 0,
        DoNotTrigger = 1,
    };

    struct PortDataEntry
    {
        PortId id;
        PortDataState state = PortDataState::Outdated;
        NodeDataPtr   data  = nullptr;

        bool isValid() const noexcept { return state == PortDataState::Valid; }
    };

    struct Entry
    {
        NodeEvalState state = NodeEvalState::Evaluated;
        QVector<PortDataEntry> portsIn = {}, portsOut = {};

        bool isDataValid(PortType type = PortType::NoType) const
        {
            bool valid = true;
            if (type != PortType::Out)
            {
                valid &= std::all_of(portsIn.begin(), portsIn.end(), [](PortDataEntry const& p){ return p.isValid(); });
            }
            if (type != PortType::In)
            {
                valid &= std::all_of(portsOut.begin(), portsOut.end(), [](PortDataEntry const& p){ return p.isValid(); });
            }
            return valid;
        }

        bool canEvaluate(Node& node) const
        {
            auto const& nodePorts = node.ports(PortType::Out);
            assert((size_t)portsOut.size() == nodePorts.size());

            return isDataValid(PortType::In) &&
                   std::all_of(portsOut.begin(), portsOut.end(),
                               [&](PortDataEntry const& port){
               auto* p = node.port(port.id);
               return (p && p->optional) || port.data;
           });
        }
    };

    struct NodeModelData
    {
        NodeModelData(NodeDataPtr _data = {}) :
            data(std::move(_data)), state(PortDataState::Valid)
        {}
        template <typename T>
        NodeModelData(std::shared_ptr<T> _data) :
            data(std::move(_data)), state(PortDataState::Valid)
        {}
        NodeModelData(PortDataEntry const& port) :
            data(port.data), state(port.state)
        {}

        NodeDataPtr data;
        PortDataState state;

        operator NodeDataPtr&() & { return data; }
        operator NodeDataPtr const&() const& { return data; }
        operator NodeDataPtr&&() && { return std::move(data); }
    };

    GraphExecutionModel(Graph& graph);

    Graph& graph();
    Graph const& graph() const;

    void beginReset();
    void endReset();
    void reset();

    bool evaluated();

    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

    bool autoEvaluate(bool enable = true);

    bool invalidateOutPorts(NodeId nodeId);
    bool invalidatePort(NodeId nodeId, PortId portId);

    IndexedNodeData nodeData(NodeId nodeId, PortType type) const;

    NodeModelData nodeData(NodeId nodeId, PortId portId) const;
    NodeModelData nodeData(NodeId nodeId, PortType type, PortIndex portIdx) const;

    bool setNodeData(NodeId nodeId, PortId portId, NodeModelData data, int option = Option::NoOption);
    bool setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeModelData data, int option = Option::NoOption);
    bool setNodeData(NodeId nodeId, PortType type, IndexedNodeData const& data, int option = Option::NoOption);

signals:

    void nodeEvaluated(NodeId nodeId);

    void graphEvaluated();

private:

    struct Impl; // helper struct to "hide" implementation details

    QHash<NodeId, Entry> m_data;

    bool m_autoEvaluate = false;

    void invalidatePort(NodeId nodeId, PortDataEntry& port);

    void triggerNodeExecution(NodeId nodeId);

private slots:

    void appendNode(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onConnectedionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onNodeEvaluated();
};

} // namespace intelli

#endif // GRAPHMODEL_H
