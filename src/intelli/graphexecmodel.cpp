/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/graphexecmodel.h"

#include "intelli/graph.h"
#include "intelli/nodefactory.h"
#include "intelli/connection.h"
#include "intelli/exec/sequentialexecutor.h"

#include <gt_logging.h>
#include <gt_exceptions.h>

using namespace intelli;

static const GraphExecutionModel::NodeDataPtr s_dummyData;

inline auto&
ports(GraphExecutionModel::Entry& entry, PortType type) noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return entry.portsIn;
    case PortType::Out:
        return entry.portsOut;
    case PortType::NoType:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

GraphExecutionModel::GraphExecutionModel(Graph& graph)
{
    setParent(&graph);

    reset();
}

Graph&
GraphExecutionModel::graph()
{
    auto tmp = static_cast<Graph*>(parent());
    assert(qobject_cast<Graph*>(parent()));
    return *tmp;
}

const Graph&
GraphExecutionModel::graph() const
{
    return const_cast<GraphExecutionModel*>(this)->graph();
}

void
GraphExecutionModel::reset()
{
    beginReset();
    endReset();
}

void
GraphExecutionModel::beginReset()
{
    for (Entry& entry : m_data)
    {
        entry.state = NodeEvalState::Evaluated;
        for (auto& data : entry.portsIn ) data.state = PortDataState::Outdated;
        for (auto& data : entry.portsOut) data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_data.clear();

    auto const& nodes = graph().nodes();
    for (auto* node : nodes)
    {
        Entry entry{};

        auto const& inPorts  = node->ports(PortType::In);
        auto const& outPorts = node->ports(PortType::Out);

        entry.portsIn.reserve(inPorts.size());
        entry.portsOut.reserve(outPorts.size());

        PortIndex idx{0};
        for (auto& _ : inPorts)
        {
            entry.portsIn.push_back(PortDataEntry{idx++});
        }

        idx = PortIndex{0};
        for (auto& _ : outPorts)
        {
            entry.portsOut.push_back(PortDataEntry{idx++});
        }

        m_data.insert(node->id(), std::move(entry));
    }
}

void
GraphExecutionModel::triggerNodeExecution(NodeId nodeId)
{
    auto& graph = this->graph();

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtWarning() << tr("Failed to trigger node execution, node %1 not found!").arg(nodeId);
        return;
    }

    if (entry->state != NodeEvalState::Evaluated)
    {
        gtDebug() << tr("Failed to trigger node execution, node %1 not marked for evaluation!").arg(nodeId);
        return;
    }

    auto* node = graph.findNode(nodeId);
    if (!node )
    {
        gtWarning() << tr("Failed to trigger node execution, node object for node id %1 not found!").arg(nodeId);
        return;
    }

    auto const& nodePorts = node->ports(PortType::Out);
    assert((size_t)entry->portsOut.size() == nodePorts.size());

    bool canEvaluate =
        std::all_of(entry->portsOut.cbegin(),
                    entry->portsOut.cend(),
                    [&](PortDataEntry const& port){
            auto const& nodePort = nodePorts.at(port.portIdx);
            return nodePort.optional || port.data;
        })
        &&
        std::all_of(entry->portsIn.cbegin(),
                    entry->portsIn.cend(),
                    [&](PortDataEntry const& port){
            return port.state == PortDataState::Valid;
        });

    if (!canEvaluate)
    {
        gtDebug() << tr("Failed to trigger node execution, node %1 is not ready yet!").arg(nodeId);
        return;
    }

    // evaluate once if node contains no out ports
    if (entry->portsOut.empty())
    {
        node->triggerNodeEvaluation();
        return;
    }

    QVarLengthArray<NodeId, 8> targetNodes;

    // evaluate all ports
    for (auto& port : entry->portsOut)
    {
        // skip valid ports
        if (port.state == PortDataState::Valid) continue;

        auto const& connections = invalidateNodeData(nodeId, port);

        // add to target nodes
        for (ConnectionId con : connections)
        {
            if (!targetNodes.contains(con.inNodeId))
            {
                targetNodes.push_back(con.inNodeId);
            }
        }
        
        node->triggerPortEvaluation(port.portIdx);
    }

    // trigger next nodes
    for (NodeId nodeId : targetNodes)
    {
        triggerNodeExecution(nodeId);
    }
}

bool
GraphExecutionModel::autoEvaluate()
{
    auto& graph = this->graph();

    auto const& cyclicNodes = intelli::cyclicNodes(graph);
    if (!cyclicNodes.isEmpty())
    {
        gtError() << QObject::tr("Cannot auto evaluate cyclic graph! The node sequence")
                  << cyclicNodes << QObject::tr("does contain a cycle!");
        return false;
    }

    auto const& nodes = graph.nodes();

    QVector<NodeId> rootNodes;

    for (auto* node : nodes)
    {
        // nodes that have no input connections are our root nodes
        if (graph.findConnections(node->id(), PortType::In).empty())
        {
            rootNodes << node->id();
        }
    }

    if (!nodes.empty() && rootNodes.empty())
    {
        gtError() << tr("Failed to find root nodes to begin graph evaluation!");
        return false;
    }

    for (NodeId nodeId : qAsConst(rootNodes))
    {
        triggerNodeExecution(nodeId);
    }

    return true;
}

GraphExecutionModel::PortDataEntry*
GraphExecutionModel::findPortDataEntry(NodeId nodeId, PortType type, PortIndex port)
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtWarning() << tr("Failed to access port data! (Invalid node %1)")
                           .arg(nodeId);
        return {};
    }

    auto& ports = ::ports(*entry, type);
    for (auto& dataEntry : ports)
    {
        if (dataEntry.portIdx == port) return &dataEntry;
    }

    gtWarning() << tr("Failed to access port data of node %1! (Port index %2 out of bounds)")
                       .arg(nodeId).arg(port) << type;
    return {};
}

GraphExecutionModel::PortDataEntry const*
GraphExecutionModel::findPortDataEntry(NodeId nodeId, PortType type, PortIndex port) const
{
    return const_cast<GraphExecutionModel*>(this)->findPortDataEntry(nodeId, type, port);
}

QVector<ConnectionId>
GraphExecutionModel::invalidateNodeData(NodeId nodeId, PortDataEntry& port)
{
    port.state = PortDataState::Outdated;

    auto connections = graph().findConnections(nodeId, PortType::Out);

    gtDebug() << "INVALIDATING Node" << nodeId << "port index" << port.portIdx
              << std::vector<ConnectionId>(connections.begin(), connections.end());

    for (auto& con : qAsConst(connections))
    {
        invalidateInNodeData(con.inNodeId, con.inPortIndex);
    }

    return connections;
}

bool
GraphExecutionModel::invalidateNodeData(NodeId nodeId)
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end()) return false;

    for (auto& port : entry->portsOut)
    {
        invalidateNodeData(nodeId, port);
    }
    return true;
}

bool
GraphExecutionModel::invalidateInNodeData(NodeId nodeId, PortIndex idx)
{
    auto* port = findPortDataEntry(nodeId, PortType::In, idx);
    if (!port) return false;

    invalidateNodeData(nodeId, *port);

    return true;
}

GraphExecutionModel::NodeDataPtr const&
GraphExecutionModel::nodeData(NodeId nodeId, PortType type, PortIndex idx) const
{
    auto* port = findPortDataEntry(nodeId, type, idx);

    if (!port)
    {
        gtWarning() << tr("Accessing node data failed! Port index %1 out of bounds!")
                           .arg(idx) << type;
        return s_dummyData;
    }

    return port->data;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeDataPtr dataPtr)
{
    auto* port = findPortDataEntry(nodeId, type, idx);
    if (!port)
    {
        gtWarning() << tr("Setting node data failed! Port index %1 out of bounds!")
                           .arg(idx) << type;
        return false;
    }

    port->data = std::move(dataPtr);
    port->state = PortDataState::Valid;

    switch (type)
    {
    case PortType::In:
        if (auto* node = graph().findNode(nodeId))
        {
            emit node->inputDataRecieved(idx);
        }
        break;
    case PortType::Out:
    {
        // forward data to target nodes
        auto const& connections = graph().findConnections(nodeId, PortType::Out, port->portIdx);

        for (ConnectionId con : connections)
        {
            setNodeData(con.inNodeId, PortType::In, con.inPortIndex, port->data);
        }
        break;
    }
    case PortType::NoType:
        throw std::logic_error("path is unreachable!");
    }

    return true;
}

