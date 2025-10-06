/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/input/graphuservariablesinput.h"
#include "intelli/data/bool.h"
#include "intelli/data/int.h"
#include "intelli/data/double.h"
#include "intelli/data/string.h"

#include <memory.h>

using namespace intelli;

namespace
{

QString variantToTypeId(QVariant::Type type)
{
    switch(type)
    {
    case QVariant::Bool:
        return typeId<BoolData>();
    case QVariant::Int:
        return typeId<IntData>();
    case QVariant::Double:
        return typeId<DoubleData>();
    case QVariant::String:
        return typeId<StringData>();
    default:
        return {};
    }
}

NodeDataPtr variantToNodeData(QVariant value)
{
    switch(value.type())
    {
    case QVariant::Bool:
        return std::make_shared<BoolData>(value.toBool());
    case QVariant::Int:
        return std::make_shared<IntData>(value.toInt());
    case QVariant::Double:
        return std::make_shared<DoubleData>(value.toDouble());
    case QVariant::String:
        return std::make_shared<StringData>(value.toString());
    default:
        return {};
    }
}

} // namespace

GraphUserVariablesInputNode::GraphUserVariablesInputNode() :
    DynamicNode("User Variables", DynamicOutputOnly)
{
    setNodeEvalMode(NodeEvalMode::Blocking);
}

GraphUserVariablesInputNode::~GraphUserVariablesInputNode() = default;

void
GraphUserVariablesInputNode::nodeEvent(NodeEvent const* e)
{
    if (e->type() == NodeEventType::DataInterfaceAvailableEvent)
    {
        return updatePorts();
    }
}

void
GraphUserVariablesInputNode::eval()
{
    GraphUserVariables const* uv = userVariables();
    if (!uv) return evalFailed();

    for (PortInfo const& port : ports(PortType::Out))
    {
        if (!uv->hasValue(port.caption)) return evalFailed();
        setNodeData(port.id(), variantToNodeData(uv->value(port.caption)));
    }
}

void
GraphUserVariablesInputNode::updatePorts()
{
    GraphUserVariables const* uv = userVariables();

    if (m_uv && m_uv != uv)
    {
        m_uv->disconnect(this);
        m_uv = nullptr;
    }

    if (!uv) return;

    m_uv = uv;

    connect(m_uv, &GraphUserVariables::variablesUpdated,
            this, &GraphUserVariablesInputNode::updatePorts,
            Qt::UniqueConnection);
    connect(m_uv, &GraphUserVariables::variablesUpdated,
            this, &Node::triggerNodeEvaluation,
            Qt::UniqueConnection);

    QStringList keys = uv->keys();

    // remove invalid ports and update existing entries
    std::vector<PortInfo> portsCopy = this->ports(PortType::Out);
    for (PortInfo const& port : portsCopy)
    {
        if (!uv->hasValue(port.caption))
        {
            bool success = removePort(port.id());
            assert(success);
            continue;
        }

        keys.removeOne(port.caption);

        QVariant const& value = uv->value(port.caption);
        QString const& typeId = variantToTypeId(value.type());
        if (typeId != port.typeId)
        {
            this->port(port.id())->typeId = variantToTypeId(value.type());
            emit portChanged(port.id());
        }
    }

    // add ports for missing entries
    for (QString const& missingKey : qAsConst(keys))
    {
        QVariant const& value = uv->value(missingKey);
        addOutPort(PortInfo{variantToTypeId(value.type())}.setCaption(missingKey));
    }
}
