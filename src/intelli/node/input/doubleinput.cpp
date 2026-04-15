/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/input/doubleinput.h>
#include <intelli/data/double.h>

#include <intelli/gui/widgets/doubleinputwidget.h>
#include <intelli/node/input/numberinputnode_utils.h>

using namespace intelli;

DoubleInputNode::DoubleInputNode() :
    Node(tr("Double Input")),
    m_value("value", tr("Value"), tr("Current value"), GtUnit::None, 0),
    m_min("min", tr("Min."), tr("Minimum value"), GtUnit::None, 0),
    m_max("max", tr("Max."), tr("Maxiumum value"), GtUnit::None, 100),
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

    m_out = addOutPort(makePort(typeId<DoubleData>())
                           .setCaptionVisible(false));

    setNodeFlag(Resizable);
    setNodeEvalMode(NodeEvalMode::Blocking);

    detail::setupNumberInputNode<DoubleInputNode,
                                 GtDoubleProperty,
                                 GtDoubleProperty,
                                 MetaEnumProperty,
                                 DoubleInputWidget::InputMode>(
        this, m_value, m_min, m_max, m_inputMode,
        [this](bool enable) { setNodeFlag(ResizableHOnly, enable); });
}

double
DoubleInputNode::value() const { return m_value.getVal(); }

void
DoubleInputNode::setValue(double value)
{
    value = m_useBounds ? gt::clamp(value, m_min.getVal(), m_max.getVal()) : value;

    if (m_value.getVal() != value)
    {
        m_value = value;
        emit triggerNodeEvaluation();
    }
}

double
DoubleInputNode::lowerBound() const { return m_min; }

void
DoubleInputNode::setLowerBound(double value)
{
    if (value > m_max) value = m_max;
    if (m_min != value)
    {
        m_min = value;
        setValue(this->value());
    }
}

double
DoubleInputNode::upperBound() const { return m_max; }

void
DoubleInputNode::setUpperBound(double value)
{
    if (value < m_min) value = m_min;
    if (m_max != value)
    {
        m_max = value;
        setValue(this->value());
    }
}

bool
DoubleInputNode::useBounds() const { return m_useBounds; }

void
DoubleInputNode::setUseBounds(bool value)
{
    // cppcheck-suppress duplicateConditionalAssign
    if (m_useBounds != value) m_useBounds = value;
}

int
DoubleInputNode::inputModeValue() const
{
    return detail::inputModeValue(m_inputMode);
}

void
DoubleInputNode::setInputModeValue(int value)
{
    detail::setInputModeValue(m_inputMode, value);
}

void
DoubleInputNode::eval()
{
    setNodeData(m_out, std::make_shared<DoubleData>(value()));
}
