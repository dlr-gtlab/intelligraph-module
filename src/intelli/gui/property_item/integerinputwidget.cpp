/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "integerinputwidget.h"

#include <QDial>
#include <QSlider>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QRegExp>
#include <QRegExpValidator>

#include <gt_logging.h>
#include <gt_lineedit.h>

#include "editableintegerlabel.h"

namespace intelli
{

IntegerInputWidget::IntegerInputWidget(int initVal, int initMin,
                                       int initMax,
                                       QWidget* parent,
                                       AbstractNumberInputWidget::InputType t) :
    AbstractNumberInputWidget(parent),
    m_maxTicks(1000),
    m_min(initMin),
    m_max(initMax),
    m_val(initVal)
{
    /// init text elements
    m_text = new GtLineEdit(this);
    m_text->setText(QString::number(m_val));
    m_text->setValidator(new QRegExpValidator(QRegExp("-?[0-9]+")));
    m_text->setMinimumWidth(30);

    m_low = new EditableIntegerLabel(QString::number(m_min), this);
    m_high = new EditableIntegerLabel(QString::number(m_max), this);

    if (t == Dial)
    {
        toDialLayout();
    }
    else if (t == SliderH)
    {
        toSliderHLayout();
    }
    else if (t == SliderV)
    {
        toSliderVLayout();
    }
    else if (t == LineEdit)
    {
        toTextBasedLayout();
    }

    auto adaptHelpingLabel = [this](EditableIntegerLabel* l)
    {
        QFont f = l->labelFont();
        f.setPointSize(8);
        f.setItalic(true);
        l->setLabelFont(f);
        l->installEventFilter(this);
    };

    m_text->installEventFilter(this);

    adaptHelpingLabel(m_low);
    adaptHelpingLabel(m_high);

    connect(m_low, SIGNAL(valueChanged(int)),
            this, SLOT(minLabelChangedReaction(int)));
    connect(m_high, SIGNAL(valueChanged(int)),
            this, SLOT(maxLabelChangedReaction(int)));

    connect(m_text, SIGNAL(editingFinished()),
            this, SLOT(valueLabelChangedReaction()));
}

void
IntegerInputWidget::onMinMaxPropertiesChanged(int val, int min, int max)
{
    bool minChange = min != m_min;
    bool maxChange = max != m_max;

    if (!minChange && !maxChange) return;

    if (min == max)
    {
        gtDebug() << "Min and max are of the same value" << min;
        return;
    }
    if (min >= max)
    {
        gtError() << tr("Min has to be smaller than max value");
        return;
    }

    if (m_dial == nullptr)
    {
        gtFatal() << "Why the hell is the dial a nullptr";
        return;
    }

    m_min = min;
    m_max = max;

    m_dial->setMinimum(min);
    m_dial->setMaximum(max);

    m_dial->setValue(val);

    if (m_low) m_low->setValue(min, false);
    if (m_high) m_high->setValue(max, false);
}

void
IntegerInputWidget::onSliderTypeChanged(QString const& t)
{
    AbstractNumberInputWidget::InputType t2 = typeFromString(t);

    if (t2 == Dial)
    {
        toDialLayout();
    }
    else if (t2 == SliderH)
    {
        toSliderHLayout();
    }
    else if (t2 == SliderV)
    {
        toSliderVLayout();
    }
    else if (t2 == LineEdit)
    {
        toTextBasedLayout();
    }
}

void
IntegerInputWidget::initDial()
{
    if (!m_dial) return;

    m_dial->setMinimum(m_min);
    m_dial->setMaximum(m_max);
    m_dial->setTracking(true);
    m_dial->setContentsMargins(0, 0, 0, 0);
    m_dial->setSingleStep(1);

    int dialStartValue = (m_max + m_min) / 2;

    auto* d = qobject_cast<QDial*>(m_dial);
    if (d) d->setNotchesVisible(true);

    m_dial->setValue(m_val);
}

void
IntegerInputWidget::toDialLayout()
{
    if (m_dial)
    {
        disconnectDial();
        delete m_dial;
    }

    if (layout()) delete layout();

    m_dial = new QDial(this);
    initDial();

    m_low->setTextAlignment(Qt::AlignHCenter);
    m_high->setTextAlignment(Qt::AlignHCenter);

    auto* layout = newDialLayout(m_dial, m_low, m_text, m_high);
    setLayout(layout);

    connectDial();
}

void
IntegerInputWidget::toSliderHLayout()
{
    if (m_dial)
    {
        disconnectDial();
        delete m_dial;
    }

    if (layout()) delete layout();

    m_dial = new QSlider(Qt::Horizontal, this);
    initDial();

    m_low->setTextAlignment(Qt::AlignLeft);
    m_high->setTextAlignment(Qt::AlignRight);

    auto* layout = newSliderHLayout(m_dial, m_low, m_text, m_high);
    setLayout(layout);

    connectDial();
}

void
IntegerInputWidget::toTextBasedLayout()
{
    toDialLayout();
    m_low->setHidden(true);
    m_high->setHidden(true);
    m_dial->setHidden(true);
}

void
IntegerInputWidget::toSliderVLayout()
{
    if (m_dial)
    {
        disconnectDial();
        delete m_dial;
    }

    if (layout()) delete layout();

    m_dial = new QSlider(Qt::Vertical, this);
    initDial();
    m_low->setTextAlignment(Qt::AlignLeft);
    m_high->setTextAlignment(Qt::AlignLeft);

    auto* layout = newSliderVLayout(m_dial, m_low, m_text, m_high);
    setLayout(layout);

    connectDial();
}

void
IntegerInputWidget::disconnectDial()
{
    disconnect(m_dial, SIGNAL(valueChanged(int)),
               this, SLOT(onDialChanged(int)));
    disconnect(m_dial, SIGNAL(sliderReleased()),
               this, SIGNAL(sliderReleased()));
}

void
IntegerInputWidget::connectDial()
{
    connect(m_dial, SIGNAL(valueChanged(int)),
            this, SLOT(onDialChanged(int)));
    connect(m_dial, SIGNAL(sliderReleased()),
            this, SIGNAL(sliderReleased()));
}

void
IntegerInputWidget::onDialChanged(int newDialVal)
{
    m_text->setText(QString::number(newDialVal));

    emit valueChanged(newDialVal);
}

void
IntegerInputWidget::minLabelChangedReaction(int newVal)
{
    if (m_dial == nullptr) return;

    m_min = newVal;
    m_dial->setMinimum(newVal);
    m_dial->setValue(m_val);

    emit onMinLabelChanged(newVal);
}

void
IntegerInputWidget::maxLabelChangedReaction(int newVal)
{
    if (m_dial == nullptr) return;

    m_max = newVal;
    m_dial->setMaximum(newVal);
    m_dial->setValue(m_val);

    emit onMaxLabelChanged(newVal);
}

void
IntegerInputWidget::valueLabelChangedReaction()
{
    if (m_dial == nullptr) return;

    QString newValString = m_text->text();
    int newVal = newValString.toInt();

    m_val = newVal;
    m_dial->setValue(newVal);

    emit onValueLabelChanged(newVal);
}

} // namespace intelli
