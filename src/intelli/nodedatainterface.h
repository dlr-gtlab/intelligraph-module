/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef NODEDATAINTERFACE_H
#define NODEDATAINTERFACE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

namespace intelli
{

class Node;
class Graph;

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

} // namespace intelli

#endif // NODEDATAINTERFACE_H
