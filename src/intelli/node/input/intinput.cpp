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

#include <intelli/private/utils.h>
#include <qslider.h>

using namespace intelli;

IntInputNode::IntInputNode() :
    Node(tr("Int Input")),
    m_value("value", tr("Value"), tr("Current value"), 0),
    m_min("min", tr("Min."), tr("Minimum value"), 0),
    m_max("max", tr("Max."), tr("Maxiumum value"), 100),
    m_useBounds("useBounds", tr("Use Min/Max"), tr("Use Min/Max bounds"), false),
    m_inputMode("mode", tr("Input Mode"), tr("Input Mode")),
    m_joystick("useJoyStick", tr("useJoystick"), tr("useJoystick"), false)
{
    registerProperty(m_value);
    registerProperty(m_min);
    registerProperty(m_max);
    registerProperty(m_useBounds);
    registerProperty(m_inputMode);
    registerProperty(m_joystick);

#ifndef GAMEPAD_USAGE
    m_joystick.hide();
#else
    m_joyStickObj = new intelli::utils::JoystickReader(this);
#endif

    m_useBounds.setReadOnly(true);
    m_value.hide();
    
    m_out = addOutPort(makePort(typeId<IntData>())
                           .setCaptionVisible(false));

    setNodeFlag(Resizable);
    setNodeEvalMode(NodeEvalMode::Blocking);

    registerWidgetFactory([this]() {
        using InputMode = IntInputWidget::InputMode;

        bool success = m_inputMode.registerEnum<InputMode>();
        assert(success);

        auto mode = m_inputMode.getEnum<InputMode>();
        
        auto* w = new IntInputWidget(mode);

        auto onRangeChanged = [this, w](){
            w->setRange(value(), lowerBound(), upperBound());
            emit nodeChanged();
            emit triggerNodeEvaluation();
        };

        auto onMinChanged = [=](){
            int newVal = w->min();
            if (lowerBound() != newVal) setLowerBound(newVal);
        };

        auto onMaxChanged = [=](){
            int newVal = w->max();
            if (upperBound() != newVal) setUpperBound(newVal);
        };

        auto onValueChanged = [=](){
            int newVal = w->value();
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
        
        connect(w, &IntInputWidget::valueComitted,
                this, onValueChanged);
        connect(w, &IntInputWidget::minChanged,
                this, onMinChanged);
        connect(w, &IntInputWidget::maxChanged,
                this, onMaxChanged);

        connect(&m_min, &GtIntProperty::changed,
                w, onRangeChanged);
        connect(&m_max, &GtIntProperty::changed,
                w, onRangeChanged);
        connect(&m_inputMode, &GtAbstractProperty::changed,
                w, updateMode);

#ifdef GAMEPAD_USAGE
        if (m_joystick)
        {
            connect(m_joyStickObj, &utils::JoystickReader::buttonPressed, [](int id)
                    {
                        gtTrace() << "Button pressed: " << id;
                    });

            connect(m_joyStickObj, &utils::JoystickReader::buttonReleased, [](int id)
                    {
                        gtTrace() << "Button released: " << id;
                    });

            connect(m_joyStickObj, &utils::JoystickReader::xAxisChange, [this, w]
                    (double percentage)
                    {
                        if (!w) return;

                        double ref = 0.75;
                        int refEval = 90;

                        int minimal = m_min;
                        int maximal = m_max;

                        double newVal = 0.;

                        if (percentage < ref)
                        {
                            newVal = m_min + (percentage / ref) * (refEval);
                        }
                        else
                        {
                            double relativePercentage = (percentage - ref) / (1. - ref);

                            int newMin = minimal + refEval;

                            newVal = newMin + relativePercentage * (maximal - newMin);
                        }

                        setValue(int(newVal));

                        if (w->slider())
                        {
                            w->slider()->setValue(int(newVal));
                        }
                    });
        }
#endif

        onRangeChanged();
        updateMode();

        return std::unique_ptr<QWidget>(w);
    });
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

void
IntInputNode::eval()
{
    setNodeData(m_out, std::make_shared<IntData>(value()));
}
