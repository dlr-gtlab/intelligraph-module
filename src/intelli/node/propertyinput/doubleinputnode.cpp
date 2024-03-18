/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "doubleinputnode.h"

#include "intelli/data/double.h"

#include "intelli/gui/property_item/doubleinputwidget.h"
#include <QLayout>

using namespace intelli;

DoubleInputNode::DoubleInputNode() :
    AbstractInputNode(tr("Double Input"),
                      std::make_unique<GtDoubleProperty>("value", tr("Value"),
                                                         tr("Current value"))),
    m_min("min", tr("Min."), tr("minimum value"), GtUnit::None, 0),
    m_max("max", tr("Max."), tr("maxiumum value"), GtUnit::None, 1),
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

    m_out = addOutPort(typeId<DoubleData>());
    port(m_out)->captionVisible = false;

    setNodeFlag(Resizable);

    registerWidgetFactory([this]() {
        DoubleInputWidget::InputType t = DoubleInputWidget::Dial;
        if (m_displayType.getVal() == m_dial.objectName())
        {
            t = DoubleInputWidget::Dial;
        }
        else if (m_displayType.getVal() == m_sliderH.objectName())
        {
            t = DoubleInputWidget::SliderH;
        }
        else if (m_displayType.getVal() == m_sliderV.objectName())
        {
            t = DoubleInputWidget::SliderV;
        }
        else if (m_displayType.getVal() == m_textDisplay.objectName())
        {
            t = DoubleInputWidget::LineEdit;
        }

        auto base = makeBaseWidget();

        auto overallW = new DoubleInputWidget(value(),
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

        auto onMinLabelChanged = [=](double newVal)
        {
            QObject::disconnect(m_minPropConnection);
            if (m_min.getVal() != newVal) m_min.setVal(newVal);
            m_minPropConnection = connect(&m_min, &GtDoubleProperty::changed,
                                          this, onMinMaxChanged);
        };

        auto onMaxLabelChanged = [=](double newVal)
        {
            QObject::disconnect(m_maxPropConnection);
            if (m_max.getVal() != newVal) m_max.setVal(newVal);
            m_maxPropConnection = connect(&m_max, &GtDoubleProperty::changed,
                                          this, onMinMaxChanged);
        };

        auto onValueLabelChanged = [=](double newVal)
        {
            if (value() != newVal) setValue(newVal);
            triggerNodeEvaluation();
        };

        auto onDisplyModeChanged = [=]()
        {
            emit displayModeChanged(m_displayType.getVal());
        };

        connect(overallW, SIGNAL(valueChanged(double)),
                this, SLOT(onWidgetValueChanges(double)));
        connect(overallW, &DoubleInputWidget::sliderReleased,
                this, &Node::triggerNodeEvaluation);

        connect(overallW, &DoubleInputWidget::onMinLabelChanged,
                this, onMinLabelChanged);
        connect(overallW, &DoubleInputWidget::onMaxLabelChanged,
                this, onMaxLabelChanged);
        connect(overallW, &DoubleInputWidget::onValueLabelChanged,
                this, onValueLabelChanged);

        connect(this, &DoubleInputNode::triggerWidgetUpdate,
                overallW, &DoubleInputWidget::onMinMaxPropertiesChanged);

        m_minPropConnection = connect(&m_min, &GtDoubleProperty::changed,
                                      this, onMinMaxChanged);
        m_maxPropConnection = connect(&m_max, &GtDoubleProperty::changed,
                                      this, onMinMaxChanged);

        connect(&m_displayType, &GtModeProperty::changed,
                                              this, onDisplyModeChanged);

        connect(this, SIGNAL(displayModeChanged(QString)),
                overallW, SLOT(onSliderTypeChanged(QString)));

        connect(overallW, SIGNAL(sizeChanged()), this, SIGNAL(nodeChanged()));

        return base;
    });
}

double
DoubleInputNode::value() const
{
    auto prop = static_cast<GtDoubleProperty*>(m_value.get());
    return prop->getVal();
}

void
DoubleInputNode::setValue(double value)
{
    auto prop = static_cast<GtDoubleProperty*>(m_value.get());
    prop->setVal(value);
}

void
DoubleInputNode::eval()
{
    setNodeData(m_out, std::make_shared<DoubleData>(value()));
}

void
DoubleInputNode::onWidgetValueChanges(double newVal)
{
    if (newVal != value()) setValue(newVal);
}

