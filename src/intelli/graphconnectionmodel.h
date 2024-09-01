/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 27.8.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_GRAPHCONNECTIONMODEL_H
#define GT_INTELLI_GRAPHCONNECTIONMODEL_H

#include <intelli/node.h>

#include <QPointer>

namespace intelli
{

/// directed acyclic graph representing connections and nodes
namespace connection_model
{

template <typename NodeId_t>
struct ConnectionDetail
{
    /// target node
    NodeId_t node;
    /// target port
    PortId port;
    /// source port
    PortId sourcePort;

    /**
     * @brief Creates an outgoing connection id.
     * @param sourceNode Source (outgoing) node
     * @return Connection id
     */
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode) const
    {
        return { sourceNode, sourcePort, node, port };
    }
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode, PortType type) const
    {
        return type == PortType::In ? toConnection(sourceNode).reversed() :
                                      toConnection(sourceNode);
    }

    /**
     * @brief Constructs an object from an outgoing connection
     * @param conId Source connection
     * @return Connection detail
     */
    static ConnectionDetail fromConnection(ConnectionId_t<NodeId_t> conId)
    {
        return { conId.inNodeId, conId.inPort, conId.outPort };
    }
};

template <typename NodeId_t>
struct ConnectionData
{
    static constexpr size_t PRE_ALLOC = 10;

    /// pointer to node
    QPointer<Node> node;
    /// adjacency lists
    QVarLengthArray<ConnectionDetail<NodeId_t>, PRE_ALLOC> predecessors = {}, successors = {};

    /**
     * @brief Returns the predecessors or successors depending on the port type
     * @param type Port type
     * @return Port vector
     */
    auto& ports(PortType type)
    {
        assert(type != PortType::NoType);
        return (type == PortType::In) ? predecessors : successors;
    }
    auto const& ports(PortType type) const
    {
        return const_cast<ConnectionData*>(this)->ports(type);
    }
};

template <typename T>
using ConnectionModel = QHash<T, ConnectionData<T>>;

template <typename Model, typename NodeId_t>
inline auto*
find(Model& model, NodeId_t nodeId)
{
    using T = decltype(&(*model.begin()));

    auto iter = model.find(nodeId);
    if (iter == model.end()) return T(nullptr);
    return &(*iter);
}

struct TraverseRecursively{};

template <typename ConnectionData_t, typename Lambda>
inline void
visit(ConnectionData_t& data, PortId sourcePort, PortType type, Lambda const& lambda)
{
    return visit(data, type, [sourcePort, type, &lambda](auto& con){
        if (con.sourcePort == sourcePort)
        {
            lambda(con);
        }
    });
}

template <typename ConnectionData_t, typename Lambda>
inline void
visit(ConnectionData_t& data, PortType type, Lambda const& lambda)
{
    static_assert(!std::is_pointer<ConnectionData_t>::value,
                  "expected ConnectionData_t& but got ConnectionData_t*");
    for (auto& con : data.ports(type))
    {
        lambda(con);
    }
}

template <typename ConnectionData_t, typename NodeId_t, typename Lambda>
inline void
visitConnections(ConnectionData_t& data, NodeId_t sourceNode, PortType type, Lambda const& lambda)
{
    static_assert(!std::is_pointer<ConnectionData_t>::value,
                  "expected ConnectionData_t& but got ConnectionData_t*");
    for (auto& con : data.ports(type))
    {
        lambda(con.toConnection(sourceNode, type));
    }
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitSuccessors(ConnectionData_t& data, PortId sourcePort, Lambda const& lambda)
{
    return visit(data, sourcePort, PortType::Out, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitSuccessors(ConnectionData_t& data, Lambda const& lambda)
{
    return visit(data, PortType::Out, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitPredecessors(ConnectionData_t& data, PortId sourcePort, Lambda const& lambda)
{
    return visit(data, sourcePort, PortType::In, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitPredecessors(ConnectionData_t& data, Lambda const& lambda)
{
    return visit(data, PortType::In, lambda);
}

template <typename ConnectionData_t>
inline bool
hasConnections(ConnectionData_t& data, PortId sourcePort, PortType type)
{
    size_t count = 0;
    visit(data, sourcePort, type, [&count](auto&){ ++count; return true; });
    return count;
}

template <typename ConnectionData_t>
inline bool
hasConnections(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::In) &&
           hasConnections(data, sourcePort, PortType::Out);
}

template <typename ConnectionData_t>
inline bool
hasSuccessors(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::Out);
}

template <typename ConnectionData_t>
inline bool
hasPredecessors(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::In);
}

template <typename ConnectionData_t>
inline bool
hasSuccessors(ConnectionData_t& data)
{
    return !data->successors.empty();
}

template <typename ConnectionData_t>
inline bool
hasPredecessors(ConnectionData_t& data)
{
    return !data->predecessors.empty();
}

template <typename ConnectionData_t, typename NodeId_t>
inline bool
containsConnection(ConnectionData_t& data,
                   NodeId_t const& sourceNode,
                   ConnectionId_t<NodeId_t> const& conId,
                   PortType type)
{
    static_assert(!std::is_pointer<ConnectionData_t>::value,
                  "expected ConnectionData_t& but got ConnectionData_t*");
    for (auto& con : data.ports(type))
    {
        if (con.toConnection(sourceNode, type) == conId) return true;
    }
    return false;
}

// operators
template <typename T>
inline bool operator==(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b)
{
    return a.node == b.node && a.port == b.port && a.sourcePort == b.sourcePort;
}

template <typename T>
inline bool operator!=(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b) { return !(a == b); }

// definitions
using GlobalConnectionModel = ConnectionModel<NodeUuid>;
using LocalConnectionModel  = ConnectionModel<NodeId>;

} // namespace connection_model

using connection_model::LocalConnectionModel;
using connection_model::GlobalConnectionModel;

} // namespace intelli;

#endif // GT_INTELLI_GRAPHCONNECTIONMODEL_H
