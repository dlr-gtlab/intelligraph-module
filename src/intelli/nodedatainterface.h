/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEDATAINTERFACE_H
#define GT_INTELLI_NODEDATAINTERFACE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

namespace intelli
{

class Node;
class Graph;

enum class NodeState
{
    Evaluated,
    RequiresReevaluation
};

namespace graph_data
{

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

#endif // GT_INTELLI_NODEDATAINTERFACE_H
