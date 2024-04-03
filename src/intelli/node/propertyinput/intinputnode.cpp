/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "intinputnode.h"

#include "intelli/gui/property_item/integerinputwidget.h"

#include "intelli/data/integer.h"

#include <QLayout>

using namespace intelli;

IntInputNode::IntInputNode() :
    AbstractInputNode("Integer Input",
                      std::make_unique<GtIntProperty>("value", tr("Value"),
                                                      tr("Current value"))),
    m_min("min", tr("Min."), tr("minimum value"), -10),
    m_max("max", tr("Max."), tr("maxiumum value"), 10),
    m_displayType("type", "type", "type"),
    m_textDisplay("Text", "Text"),
    m_dial("dial", "dial"),
    m_sliderH("sliderH", "Slider H"),
    m_sliderV("sliderV", "Slider V")
{
    registerProperty(m_min);
    registerProperty(m_max);

    m_displayType.registerSubProperty(m_textDisplay);
    m_displayType.registerSubProperty(m_dial);
    m_displayType.registerSubProperty(m_sliderH);
    m_displayType.registerSubProperty(m_sliderV);

    registerProperty(m_displayType);

    m_value->hide();
    
    m_out = addOutPort(typeId<IntData>());
    port(m_out)->captionVisible = false;

    setNodeFlag(Resizable);

    registerWidgetFactory([this]() {
        IntegerInputWidget::InputType t = IntegerInputWidget::Dial;
        if (m_displayType.getVal() == m_dial.objectName())
        {
            t = IntegerInputWidget::Dial;
        }
        else if (m_displayType.getVal() == m_sliderH.objectName())
        {
            t = IntegerInputWidget::SliderH;
        }
        else if (m_displayType.getVal() == m_sliderV.objectName())
        {
            t = IntegerInputWidget::SliderV;
        }
        else if (m_displayType.getVal() == m_textDisplay.objectName())
        {
            t = IntegerInputWidget::LineEdit;
        }

        auto base = makeBaseWidget();

        auto overallW = new IntegerInputWidget(value(),
                                                 m_min.getVal(),
                                                 m_max.getVal(),
                                                 base.get(),
                                                 t);

        base->layout()->addWidget(overallW);

        auto onMinMaxChanged = [this]()
        {
            emit triggerWidgetUpdate(value(),
                                     m_min.getVal(),
                                     m_max.getVal());

            emit triggerNodeEvaluation();
        };

        auto onMinLabelChanged = [=](int newVal)
        {
            QObject::disconnect(m_minPropConnection);
            if (m_min.getVal() != newVal) m_min.setVal(newVal);
            m_minPropConnection = connect(&m_min, &GtIntProperty::changed,
                                          this, onMinMaxChanged);
        };

        auto onMaxLabelChanged = [=](int newVal)
        {
            QObject::disconnect(m_maxPropConnection);
            if (m_max.getVal() != newVal) m_max.setVal(newVal);
            m_maxPropConnection = connect(&m_max, &GtIntProperty::changed,
                                          this, onMinMaxChanged);
        };

        auto onValueLabelChanged = [=](int newVal)
        {
            if (value() != newVal) setValue(newVal);
            emit triggerNodeEvaluation();
        };

        auto onDisplyModeChanged = [=]()
        {
            emit displayModeChanged(m_displayType.getVal());
        };

        connect(overallW, SIGNAL(valueChanged(int)),
                this, SLOT(onWidgetValueChanges(int)));
        connect(overallW, &IntegerInputWidget::sliderReleased,
                this, &Node::triggerNodeEvaluation);

        connect(overallW, &IntegerInputWidget::onMinLabelChanged,
                this, onMinLabelChanged);
        connect(overallW, &IntegerInputWidget::onMaxLabelChanged,
                this, onMaxLabelChanged);
        connect(overallW, &IntegerInputWidget::onValueLabelChanged,
                this, onValueLabelChanged);

        connect(this, &IntInputNode::triggerWidgetUpdate,
                overallW, &IntegerInputWidget::onMinMaxPropertiesChanged);

        m_minPropConnection = connect(&m_min, &GtIntProperty::changed,
                                      this, onMinMaxChanged);
        m_maxPropConnection = connect(&m_max, &GtIntProperty::changed,
                                      this, onMinMaxChanged);

        connect(&m_displayType, &GtModeProperty::changed,
                                              this, onDisplyModeChanged);

        connect(this, SIGNAL(displayModeChanged(QString)),
                overallW, SLOT(onSliderTypeChanged(QString)));

        connect(overallW, SIGNAL(sizeChanged()), this, SIGNAL(nodeChanged()));

        return base;
    });
}

int
IntInputNode::value() const
{
    auto prop = static_cast<GtIntProperty*>(m_value.get());
    return prop->getVal();
}

void
IntInputNode::setValue(int value)
{
    auto prop = static_cast<GtIntProperty*>(m_value.get());
    prop->setVal(value);
}

void
IntInputNode::eval()
{
    setNodeData(m_out, std::make_shared<IntData>(value()));
}

void
IntInputNode::onWidgetValueChanges(int newVal)
{
    if (newVal != value()) setValue(newVal);
}
