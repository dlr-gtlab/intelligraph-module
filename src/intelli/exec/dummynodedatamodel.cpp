/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/exec/dummynodedatamodel.h>
#include <intelli/node.h>
#include <intelli/private/utils.h>

using namespace intelli;

DummyNodeDataModel::DummyNodeDataModel(Node& node) :
    m_node(&node)
{
    auto const& inPorts  = node.ports(PortType::In);
    auto const& outPorts = node.ports(PortType::Out);

    m_data.portsIn.reserve(inPorts.size());
    m_data.portsOut.reserve(outPorts.size());

    for (auto& port : inPorts)
    {
        m_data.portsIn.push_back({port.id()});
    }

    for (auto& port : outPorts)
    {
        m_data.portsOut.push_back({port.id()});
    }

    exec::setNodeDataInterface(node, this);
}

DummyNodeDataModel::~DummyNodeDataModel()
{
    assert(m_node);
    exec::setNodeDataInterface(*m_node, nullptr);
}

NodeDataPtrList
DummyNodeDataModel::nodeData(PortType type) const
{
    assert(m_node);
    auto const& ports = m_data.ports(type);

    NodeDataPtrList data;
    std::transform(ports.begin(),
                   ports.end(),
                   std::back_inserter(data),
                   [](auto& port){
        using T = typename NodeDataPtrList::value_type;
        return T{port.portId, port.data};
    });
    return data;
}

NodeDataSet
DummyNodeDataModel::nodeData(NodeUuid const& nodeUuid,
                             PortId portId) const
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                 "was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return {};
    }

    auto* item = m_data.findPort(portId);
    if (!item)
    {
        gtWarning() << QObject::tr("DummyDataModel: Failed to access data of '%1' (%2), "
                                   "port %4 not found!")
                           .arg(relativeNodePath(*m_node))
                           .arg(m_node->id())
                           .arg(portId);
        return {};
    }

    return item->data;
}

NodeDataPtrList
DummyNodeDataModel::nodeData(NodeUuid const& nodeUuid,
                             PortType type) const
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                 "was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return {};
    }

    return nodeData(type);
}

bool
DummyNodeDataModel::setNodeData(PortId portId,
                                NodeDataSet data)
{
    assert(m_node);
    return setNodeData(m_node->uuid(), portId, std::move(data));
}

bool
DummyNodeDataModel::setNodeData(PortType type,
                                NodeDataPtrList const& data)
{
    assert(m_node);
    for (auto& d : data)
    {
        if (!setNodeData(d.first, std::move(d.second)))
        {
            return false;
        }
    }
    return true;
}

bool
DummyNodeDataModel::setNodeData(NodeUuid const& nodeUuid,
                                PortId portId,
                                NodeDataSet data)
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                 "was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return false;
    }

    auto* item = m_data.findPort(portId);
    if (!item)
    {
        gtWarning() << QObject::tr("DummyDataModel: Failed to set data of %1 (%2:%3), "
                                   "port %4 not found!")
                           .arg(nodeUuid)
                           .arg(m_node->id(), 2)
                           .arg(m_node->caption())
                           .arg(portId);
        return false;
    }

    item->data =std::move(data);
    return true;
}

bool
DummyNodeDataModel::setNodeData(NodeUuid const& nodeUuid,
                                PortType type,
                                NodeDataPtrList const& data)
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                 "was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return false;
    }

    return setNodeData(type, data);
}

NodeEvalState
DummyNodeDataModel::nodeEvalState(const NodeUuid& nodeUuid) const
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                 "was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return NodeEvalState::Invalid;
    }

    return NodeEvalState::Evaluating;
}

void
DummyNodeDataModel::setNodeEvaluationFailed(NodeUuid const& nodeUuid)
{
    assert(m_node);
    if (nodeUuid != m_node->uuid())
    {
        gtError() << QObject::tr("DummyDataModel: Failed to mark evaluation "
                                 "of node %1 as failed, was expecting node %2!")
                         .arg(nodeUuid, m_node->uuid());
        return;
    }

    m_success = false;
}
