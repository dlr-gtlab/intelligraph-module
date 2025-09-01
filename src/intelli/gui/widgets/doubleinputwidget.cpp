/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/gui/widgets/doubleinputwidget.h>
#include <intelli/gui/widgets/editablelabel.h>
#include <intelli/utilities.h>

#include <gt_utilities.h>
#include <gt_lineedit.h>
#include <gt_regexp.h>
#include <gt_logging.h>

#include <QRegExp>
#include <QRegExpValidator>
#include <QDial>
#include <QSlider>

using namespace intelli;

DoubleInputWidget::DoubleInputWidget(InputMode mode,
                                     QWidget* parent) :
    AbstractNumberInputWidget(mode,
                              new EditableIntegerLabel("", nullptr),
                              new EditableIntegerLabel("", nullptr),
                              parent)
{
    valueEdit()->setValidator(new QRegExpValidator(gt::re::forDoubles(), this));

    // emulate double slider
    slider()->setMinimum(0);
    slider()->setMaximum(ticks());

    dial()->setMinimum(0);
    dial()->setMaximum(ticks());
    dial()->setNotchesVisible(false);
}

double
DoubleInputWidget::min() const { return m_min; }

double
DoubleInputWidget::max() const { return m_max; }

int
DoubleInputWidget::ticks() const { return m_ticks; }

void
DoubleInputWidget::applyRange(QVariant const& valueV,
                              QVariant const& minV,
                              QVariant const& maxV)
{
    double value = valueV.toDouble();
    double min = minV.toDouble();
    double max = maxV.toDouble();

    if (min > max)
    {
        gtError().medium() << tr("Min has to be smaller than max value (%1 vs %2")
                                  .arg(min).arg(max);
        min = max;
    }

    m_min = min;
    m_max = max;

    if (useBounds()) value = gt::clamp(value, min, max);

    int dvalue = utils::map<int>(value, {min, max}, {0, ticks()});

    dial()->setValue(dvalue);
    slider()->setValue(dvalue);

    low()->setValue(min, false);
    high()->setValue(max, false);
    valueEdit()->setText(QString::number(value));
}

void
DoubleInputWidget::commitSliderValueChange(int value)
{
    double dvalue = utils::map<double>(value, {0, ticks()}, {min(), max()});

    valueEdit()->setText(QString::number(dvalue));
}

void
DoubleInputWidget::commitMinValueChange()
{
    double value = low()->value<double>();
    if (value > m_max)
    {
        value = m_max;
        low()->setValue(value, false);
    }

    m_min = value;
}

void
DoubleInputWidget::commitMaxValueChange()
{
    double value = high()->value<double>();
    if (value < m_min)
    {
        value = m_min;
        low()->setValue(value, false);
    }

    m_max = value;
}

void
DoubleInputWidget::commitValueChange()
{
    double value = this->value<double>();
    if (useBounds()) value = gt::clamp(value, m_min, m_max);

    int dvalue = utils::map<int>(value, {min(), max()}, {0, ticks()});

    dial()->setValue(dvalue);
    slider()->setValue(dvalue);
    valueEdit()->setText(QString::number(value));
}
