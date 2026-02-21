/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/input/stringinput.h>
#include <intelli/data/string.h>

using namespace intelli;

StringInputNode::StringInputNode() :
    Node("String Input"),
    m_value("value", tr("Value"), tr("Current value"))
{
    registerProperty(m_value);

    setNodeFlag(ResizableHOnly, true);
    setNodeEvalMode(NodeEvalMode::Blocking);

    m_out = addOutPort(makePort(typeId<StringData>())
                           .setCaptionVisible(false));

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
    connect(&m_value, &GtAbstractProperty::changed,
            this, [this]() { emit valueChanged(m_value.get()); });

}

QString const&
StringInputNode::value() const
{
    return m_value.get();
}

void
StringInputNode::setValue(QString value)
{
    if (m_value.get() == value) return;
    m_value = std::move(value);
}

void
StringInputNode::eval()
{
    setNodeData(m_out, std::make_shared<intelli::StringData>(value()));
}
