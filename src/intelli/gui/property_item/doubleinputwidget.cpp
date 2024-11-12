/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "doubleinputwidget.h"

#include <QDial>
#include <QSlider>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QRegExpValidator>

#include <gt_regexp.h>
#include <gt_lineedit.h>
#include <gt_logging.h>


#include "editabledoublelabel.h"

namespace intelli
{

DoubleInputWidget::DoubleInputWidget(double initVal, double initMin,
                                     double initMax,
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
    m_text->setMinimumWidth(30);
    m_text->setValidator(new QRegExpValidator(gt::re::forDoubles()));

    m_low = new EditableDoubleLabel(QString::number(m_min), this);
    m_high = new EditableDoubleLabel(QString::number(m_max), this);

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

    auto adaptHelpingLabel = [this](EditableDoubleLabel* l)
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

    connect(m_low, SIGNAL(valueChanged(double)),
            this, SLOT(minLabelChangedReaction(double)));
    connect(m_high, SIGNAL(valueChanged(double)),
            this, SLOT(maxLabelChangedReaction(double)));

    connect(m_text, SIGNAL(editingFinished()),
            this, SLOT(valueLabelChangedReaction()));
}

void
DoubleInputWidget::onMinMaxPropertiesChanged(double val, double min, double max)
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

    int dialValue = (val - m_min) / (m_max - m_min) * m_maxTicks;

    m_dial->setValue(dialValue);

    if (m_low) m_low->setValue(min, false);
    if (m_high) m_high->setValue(max, false);
}

void
DoubleInputWidget::onSliderTypeChanged(QString const& t)
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
DoubleInputWidget::initDial()
{
    if (!m_dial) return;

    m_dial->setMinimum(0);
    m_dial->setMaximum(m_maxTicks);
    m_dial->setTracking(true);
    m_dial->setContentsMargins(0, 0, 0, 0);

    int dialStartValue = m_maxTicks / 2;

    if (m_min >= m_max)
    {
        gtError() << tr("Min has to be smaller than max value");
    }
    else
    {
        dialStartValue = (m_val - m_min)
                        / (m_max - m_min) * m_maxTicks;
    }

    m_dial->setValue(dialStartValue);
}

void
DoubleInputWidget::toDialLayout()
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

    QLayout* layout = newDialLayout(m_dial, m_low, m_text, m_high);
    setLayout(layout);

    connectDial();
}

void
DoubleInputWidget::toSliderHLayout()
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

    connectDial();

    setLayout(layout);
}

void
DoubleInputWidget::toTextBasedLayout()
{
    toDialLayout();
    m_low->setHidden(true);
    m_high->setHidden(true);
    m_dial->setHidden(true);
}

void
DoubleInputWidget::toSliderVLayout()
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
DoubleInputWidget::disconnectDial()
{
    disconnect(m_dial, SIGNAL(sliderReleased()),
               this, SLOT(onDialChanged()));
}

void
DoubleInputWidget::connectDial()
{
    connect(m_dial, SIGNAL(sliderReleased()),
            this, SLOT(onDialChanged()));
}

void
DoubleInputWidget::onDialChanged()
{
    if (m_dial == nullptr) return;

    int newDialVal = m_dial->value();

    double newVal =
            m_min + double(newDialVal) / double(m_maxTicks) * (m_max - m_min);

    m_text->setText(QString::number(newVal));

    emit valueChanged(newVal);
}

void
DoubleInputWidget::minLabelChangedReaction(double newVal)
{
    if (m_dial == nullptr) return;

    double oldVal = m_text->text().toDouble();
    m_min = newVal;
    int dialValue = (oldVal - m_min) / (m_max - m_min) * m_maxTicks;
    m_dial->setValue(dialValue);

    emit onMinLabelChanged(newVal);
}

void
DoubleInputWidget::maxLabelChangedReaction(double newVal)
{
    if (m_dial == nullptr) return;

    double oldVal = m_text->text().toDouble();
    m_max = newVal;
    int dialValue = (oldVal - m_min) / (m_max - m_min) * m_maxTicks;
    m_dial->setValue(dialValue);

    emit onMaxLabelChanged(newVal);
}

void
DoubleInputWidget::valueLabelChangedReaction()
{
    if (m_dial == nullptr) return;

    QString newValString = m_text->text();

    double newVal = newValString.toDouble();

    double helper = (newVal - m_min) / (m_max - m_min) * m_maxTicks;

    int dialValue = int(helper);
    m_dial->setValue(dialValue);
    m_val = newVal;

    emit onValueLabelChanged(newVal);
}

} /// namespace intelli
