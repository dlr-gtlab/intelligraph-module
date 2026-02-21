/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/input/intinput.h>
#include <intelli/data/int.h>

#include <intelli/gui/widgets/intinputwidget.h>
#include <intelli/node/input/numberinputnode_utils.h>

using namespace intelli;

IntInputNode::IntInputNode() :
    Node(tr("Int Input")),
    m_value("value", tr("Value"), tr("Current value"), 0),
    m_min("min", tr("Min."), tr("Minimum value"), 0),
    m_max("max", tr("Max."), tr("Maxiumum value"), 100),
    m_useBounds("useBounds", tr("Use Min/Max"), tr("Use Min/Max bounds"), false),
    m_inputMode("mode", tr("Input Mode"), tr("Input Mode"))
{
    registerProperty(m_value);
    registerProperty(m_min);
    registerProperty(m_max);
    registerProperty(m_useBounds);
    registerProperty(m_inputMode);

    m_useBounds.setReadOnly(true);
    m_value.hide();
    
    m_out = addOutPort(makePort(typeId<IntData>())
                           .setCaptionVisible(false));

    setNodeFlag(Resizable);
    setNodeEvalMode(NodeEvalMode::Blocking);

    detail::setupNumberInputNode<IntInputNode,
                                 GtIntProperty,
                                 GtIntProperty,
                                 MetaEnumProperty,
                                 IntInputWidget::InputMode>(
        this, m_value, m_min, m_max, m_inputMode,
        [this](bool enable) { setNodeFlag(ResizableHOnly, enable); });
}

int
IntInputNode::value() const { return m_value.getVal(); }

void
IntInputNode::setValue(int value)
{
    value = m_useBounds ? gt::clamp(value, m_min.getVal(), m_max.getVal()) : value;

    if (m_value.getVal() != value)
    {
        m_value = value;
        emit triggerNodeEvaluation();
    }
}

int
IntInputNode::lowerBound() const { return m_min; }

void
IntInputNode::setLowerBound(int value)
{
    if (value > m_max) value = m_max;
    if (m_min != value)
    {
        m_min = value;
        setValue(this->value());
    }
}

int
IntInputNode::upperBound() const { return m_max; }

void
IntInputNode::setUpperBound(int value)
{
    if (value < m_min) value = m_min;
    if (m_max != value)
    {
        m_max = value;
        setValue(this->value());
    }
}

bool
IntInputNode::useBounds() const { return m_useBounds; }

void
IntInputNode::setUseBounds(bool value)
{
    // cppcheck-suppress duplicateConditionalAssign
    if (m_useBounds != value) m_useBounds = value;
}

int
IntInputNode::inputModeValue() const
{
    return detail::inputModeValue(m_inputMode);
}

void
IntInputNode::setInputModeValue(int value)
{
    detail::setInputModeValue(m_inputMode, value);
}

void
IntInputNode::eval()
{
    setNodeData(m_out, std::make_shared<IntData>(value()));
}
