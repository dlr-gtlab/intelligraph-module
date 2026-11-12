/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEEVENT_H
#define GT_INTELLI_NODEEVENT_H

#include <intelli/globals.h>

#define DECLARE_NODE_EVENT_TYPE(TYPE) \
  static constexpr NodeEventType Type = NodeEventType::TYPE; \
  NodeEventType type() const noexcept { return Type; }

namespace intelli
{

/// Enums inidacting of node event
enum class NodeEventType : unsigned
{
    Unknown = 0,
    /// Event is emitted once `nodeDataInterface` yields a valid pointer
    /// (not emitted if `nodeDataInterface` changes).
    /// Can be used to initialize a node.
    DataInterfaceAvailable,

    NodeInvalidated
};

/// Base class for node specific events
class NodeEvent
{
public:
    virtual NodeEventType type() const noexcept = 0;

    template <typename T>
    std::remove_pointer_t<T> const* cast() const noexcept
    {
        return (type() == std::remove_pointer_t<T>::Type) ?
                   static_cast<std::remove_pointer_t<T> const*>(this) : nullptr;
    }
};

/// Emitted once a data interface becomes available
class DataInterfaceAvailableEvent : public NodeEvent
{
public:
    DECLARE_NODE_EVENT_TYPE(DataInterfaceAvailable);
};

/// Emitted once the node is invalidated
class NodeInvalidatedEvent : public NodeEvent
{
public:
    DECLARE_NODE_EVENT_TYPE(NodeInvalidated);
};

} // namespace intelli

#endif // GT_INTELLI_NODEEVENT_H
