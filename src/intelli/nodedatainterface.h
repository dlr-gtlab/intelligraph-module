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

#endif // NODEDATAINTERFACE_H
