/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/input/boolinput.h>
#include <intelli/data/bool.h>

using namespace intelli;

BoolInputNode::BoolInputNode() :
    Node("Bool Input"),
    m_value("value", tr("Value"), tr("Current Value")),
    m_displayMode("displayMode",
                  tr("Display Mode"),
                  tr("Display Mode"))
{
    registerProperty(m_value);
    registerProperty(m_displayMode);

    setNodeEvalMode(NodeEvalMode::Blocking);

    m_out = addOutPort(makePort(typeId<BoolData>())
                           .setCaptionVisible(false));

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

bool
BoolInputNode::value() const
{
    return m_value.get();
}

void
BoolInputNode::setValue(bool value)
{
    m_value = value;
}

void
BoolInputNode::eval()
{
    setNodeData(m_out, std::make_shared<BoolData>(value()));
}
