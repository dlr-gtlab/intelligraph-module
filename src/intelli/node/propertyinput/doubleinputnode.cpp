/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "doubleinputnode.h"

#include "intelli/data/double.h"

#include "intelli/gui/property_item/doubleinputwidget.h"
#include <QLayout>

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

    m_out = addOutPort(makePort(typeId<DoubleData>()).setCaptionVisible(false));

    setNodeFlag(Resizable);
    setNodeEvalMode(NodeEvalMode::Blocking);

    registerWidgetFactory([this]() {
        using InputMode = DoubleInputWidget::InputMode;

        bool success = m_inputMode.registerEnum<InputMode>();
        assert(success);

        auto mode = m_inputMode.getEnum<InputMode>();

        auto w = new DoubleInputWidget(mode);

        auto onRangeChanged = [this, w](){
            w->setRange(value(), lowerBound(), upperBound());
            emit nodeChanged();
            emit triggerNodeEvaluation();
        };

        auto onMinChanged = [=](){
            double newVal = w->min();
            if (lowerBound() != newVal) setLowerBound(newVal);
        };

        auto onMaxChanged = [=](){
            double newVal = w->max();
            if (upperBound() != newVal) setUpperBound(newVal);
        };

        auto onValueChanged = [=](){
            double newVal = w->value();
            if (value() != newVal)
            {
                setValue(newVal);
                emit triggerNodeEvaluation();
            }
        };

        auto const updateMode= [this, w]() {
            w->setInputMode(m_inputMode.getEnum<InputMode>());

            setUseBounds(w->useBounds());

            switch (w->inputMode())
            {
            case InputMode::SliderH:
            case InputMode::LineEditBound:
            case InputMode::LineEditUnbound:
                setNodeFlag(ResizableHOnly, true);
                break;
            default:
                setNodeFlag(ResizableHOnly, false);
                break;
            }

            emit nodeChanged();
        };

        connect(w, &DoubleInputWidget::valueComitted,
                this, onValueChanged);
        connect(w, &DoubleInputWidget::minChanged,
                this, onMinChanged);
        connect(w, &DoubleInputWidget::maxChanged,
                this, onMaxChanged);

        connect(&m_min, &GtDoubleProperty::changed,
                w, onRangeChanged);
        connect(&m_max, &GtDoubleProperty::changed,
                w, onRangeChanged);
        connect(&m_inputMode, &GtAbstractProperty::changed,
                w, updateMode);

        onRangeChanged();
        updateMode();

        return std::unique_ptr<QWidget>(w);
    });
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
    if (m_useBounds != value) m_useBounds = value;
}

void
DoubleInputNode::eval()
{
    setNodeData(m_out, std::make_shared<DoubleData>(value()));
}

