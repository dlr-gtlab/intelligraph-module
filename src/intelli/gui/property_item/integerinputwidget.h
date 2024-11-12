/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_INTEGERINPUTWIDGET_H
#define GT_INTELLI_INTEGERINPUTWIDGET_H

#include "abstractnumberinputwidget.h"

class QAbstractSlider;
class QLabel;
class GtLineEdit;

namespace intelli
{
class EditableIntegerLabel;

class IntegerInputWidget : public AbstractNumberInputWidget
{
    Q_OBJECT
public:

    Q_INVOKABLE IntegerInputWidget(int initVal, int initMin,
                                   int initMax,
                                   QWidget* parent = nullptr,
                                   AbstractNumberInputWidget::InputType t = Dial);

signals:
    void valueChanged(int newVal);

    void onMinLabelChanged(int newVal);

    void onMaxLabelChanged(int newVal);

    void onValueLabelChanged(int newVal);

public slots:
    void onMinMaxPropertiesChanged(int val, int min, int max);

    void onSliderTypeChanged(QString const& t);

private:
    int m_maxTicks;

    QAbstractSlider* m_dial{nullptr};

    GtLineEdit* m_text{nullptr};

    EditableIntegerLabel* m_low{nullptr};

    EditableIntegerLabel* m_high{nullptr};

    int m_min;

    int m_max;

    int m_val;

    void initDial();

    void toDialLayout();

    void toSliderVLayout();

    void toSliderHLayout();

    void toTextBasedLayout();

    void disconnectDial();

    void connectDial();

private slots:
    void onDialChanged();

    void minLabelChangedReaction(int newVal);

    void maxLabelChangedReaction(int newVal);

    void valueLabelChangedReaction();
};

} // namespace intelli

#endif // GT_INTELLI_INTEGERINPUTWIDGET_H
