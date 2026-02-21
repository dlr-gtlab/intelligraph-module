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

#include <QString>

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

    bool success = m_inputMode.registerEnum<IntInputWidget::InputMode>();
    assert(success);

    auto updateResizability = [this]() {
        using InputMode = IntInputWidget::InputMode;
        switch (static_cast<InputMode>(inputModeValue()))
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
    };

    connect(&m_value, &GtIntProperty::changed,
            this, [this]() { emit rangeChanged(); });
    connect(&m_min, &GtIntProperty::changed,
            this, [this]() {
                emit rangeChanged();
                emit nodeChanged();
                emit triggerNodeEvaluation();
            });
    connect(&m_max, &GtIntProperty::changed,
            this, [this]() {
                emit rangeChanged();
                emit nodeChanged();
                emit triggerNodeEvaluation();
            });
    connect(&m_inputMode, &GtAbstractProperty::changed,
            this, [this, updateResizability]() {
                updateResizability();
                emit inputModeChanged();
                emit nodeChanged();
            });

    updateResizability();
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
    if (!m_inputMode.isInitialized()) return 0;
    return m_inputMode.getMetaEnum().keyToValue(m_inputMode.getVal().toUtf8());
}

void
IntInputNode::setInputModeValue(int value)
{
    if (!m_inputMode.isInitialized()) return;
    const char* key = m_inputMode.getMetaEnum().valueToKey(value);
    if (!key) return;
    if (m_inputMode.getVal() == QLatin1String(key)) return;
    bool success = true;
    m_inputMode.setVal(key, &success);
}

void
IntInputNode::eval()
{
    setNodeData(m_out, std::make_shared<IntData>(value()));
}
