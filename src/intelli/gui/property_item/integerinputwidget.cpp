/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/gui/property_item/integerinputwidget.h>
#include <intelli/gui/property_item/editableintegerlabel.h>
#include <intelli/globals.h>

#include <gt_utilities.h>
#include <gt_lineedit.h>
#include <gt_logging.h>

#include <QRegExp>
#include <QRegExpValidator>
#include <QDial>
#include <QSlider>

using namespace intelli;

IntegerInputWidget::IntegerInputWidget(InputMode mode,
                                       QWidget* parent) :
    AbstractNumberInputWidget(mode,
                              new EditableIntegerLabel("", nullptr),
                              new EditableIntegerLabel("", nullptr),
                              parent)
{
    valueEdit()->setValidator(new QRegExpValidator(QRegExp("-?[0-9]+")));
}

int
IntegerInputWidget::min() const { return m_min; }

int
IntegerInputWidget::max() const { return m_max; }

void
IntegerInputWidget::applyRange(QVariant const& valueV,
                               QVariant const& minV,
                               QVariant const& maxV)
{
    int value = valueV.toInt();
    int min = minV.toInt();
    int max = maxV.toInt();

    if (min > max)
    {
        gtError().medium() << tr("Min has to be smaller than max value (%1 vs %2")
                                  .arg(min).arg(max);
        min = max;
    }

    m_min = min;
    m_max = max;

    if (useBounds()) value = gt::clamp(value, min, max);

    dial()->setMinimum(min);
    dial()->setMaximum(max);
    dial()->setValue(value);

    slider()->setMinimum(min);
    slider()->setMaximum(max);
    slider()->setValue(value);

    low()->setValue(min, false);
    high()->setValue(max, false);
    valueEdit()->setText(QString::number(value));
}

void
IntegerInputWidget::commitSliderValueChange(int value)
{
    valueEdit()->setText(QString::number(value));
}

void
IntegerInputWidget::commitMinValueChange()
{
    int value = low()->value<int>();
    if (value > m_max)
    {
        value = m_max;
        low()->setValue(value, false);
    }

    m_min = value;

    dial()->setMinimum(value);
    dial()->setValue(dial()->value());

    slider()->setMinimum(value);
    slider()->setValue(slider()->value());
}

void
IntegerInputWidget::commitMaxValueChange()
{
    int value = high()->value<int>();
    if (value < m_min)
    {
        value = m_min;
        low()->setValue(value, false);
    }

    m_max = value;

    dial()->setMaximum(value);
    dial()->setValue(dial()->value());

    slider()->setMaximum(value);
    slider()->setValue(slider()->value());
}

void
IntegerInputWidget::commitValueChange()
{
    int value = valueEdit()->text().toInt();
    if (useBounds()) value = gt::clamp(value, m_min, m_max);

    dial()->setValue(value);
    slider()->setValue(value);
    valueEdit()->setText(QString::number(value));
}
