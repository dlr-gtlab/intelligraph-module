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

enum class NodeEvalState
{
    Invalid = 0,
    Outdated,
    Evaluating,
    Paused,
    Valid
};

enum class NodeState
{
    Evaluated,
    RequiresReevaluation
};

namespace graph_data
{

struct PortEntry;

/// helper struct representing node data and its validity state
struct NodeDataSet
{
    NodeDataSet(std::nullptr_t) :
        ptr(nullptr), state(PortDataState::Outdated)
    {}
    NodeDataSet(NodeDataPtr _data = {}) :
        ptr(std::move(_data)), state(PortDataState::Valid)
    {}
    template <typename T>
    NodeDataSet(std::shared_ptr<T> _data) :
        ptr(std::move(_data)), state(PortDataState::Valid)
    {}

    /// actual node data
    NodeDataPtr ptr;
    /// data state
    PortDataState state;

    operator NodeDataPtr&() & { return ptr; }
    operator NodeDataPtr() && { return std::move(ptr); }
    operator NodeDataPtr const&() const& { return ptr; }

    template <typename T>
    inline auto value() const noexcept { return qobject_pointer_cast<T const>(ptr); }
};

struct PortEntry
{
    /// referenced port
    PortId id;
    /// actual data at port
    NodeDataSet data{nullptr};
};

struct Entry
{
    /// in and out ports
    QVector<PortEntry> portsIn{}, portsOut{};

    NodeState state = NodeState::RequiresReevaluation;
};

using DataModel [[deprecated("Use 'GraphData' instead")]] = QHash<NodeId, Entry>;
using GraphData = QHash<NodeId, Entry>;

} // namesace graph_data

using graph_data::GraphData;
using graph_data::NodeDataSet;

/**
 * @brief The NodeDataInterface class.
 * Interface to access and set the data of a node port
 */
class NodeDataInterface
{
public:

    virtual ~NodeDataInterface() = default;

    virtual NodeDataSet nodeData(NodeId nodeId, PortId portId) const = 0;

    virtual bool setNodeData(NodeId nodeId, PortId portId, NodeDataSet data) = 0;
};

} // namespace intelli

#endif // NODEDATAINTERFACE_H
