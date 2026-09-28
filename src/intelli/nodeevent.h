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

namespace intelli
{

/// Enums inidacting of node event
enum NodeEventType
{
    UnkownEvent = 0,
    /// Event is emitted once `nodeDataInterface` yields a valid pointer
    /// (not emitted if `nodeDataInterface` chnages).
    /// Can be used to initialize a node.
    DataInterfaceAvailableEvent
};

/// Base class for node specific events
class NodeEvent
{
    NodeEventType m_type = UnkownEvent;
public:
    explicit NodeEvent(NodeEventType type) : m_type(type) {}
    NodeEventType type() const { return m_type; }
};

} // namespace intelli

#endif // GT_INTELLI_NODEEVENT_H
