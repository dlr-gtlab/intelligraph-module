/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_DOUBLEINPUTWIDGET_H
#define GT_INTELLI_DOUBLEINPUTWIDGET_H

#include "abstractnumberinputwidget.h"

class QAbstractSlider;
class QLabel;
class GtLineEdit;

namespace intelli
{
class EditableDoubleLabel;

class DoubleInputWidget : public AbstractNumberInputWidget
{
    Q_OBJECT
public:

    Q_INVOKABLE DoubleInputWidget(double initVal, double initMin,
                                  double initMax,
                                  QWidget* parent = nullptr,
                                  AbstractNumberInputWidget::InputType t = Dial);

signals:
    void valueChanged(double newVal);

    void onMinLabelChanged(double newVal);

    void onMaxLabelChanged(double newVal);

    void onValueLabelChanged(double newVal);

public slots:
    void onMinMaxPropertiesChanged(double val, double min, double max);

    void onSliderTypeChanged(QString const& t);

private:
    int m_maxTicks;

    QAbstractSlider* m_dial{nullptr};

    GtLineEdit* m_text{nullptr};

    EditableDoubleLabel* m_low{nullptr};

    EditableDoubleLabel* m_high{nullptr};

    double m_min;

    double m_max;

    double m_val;

    void initDial();

    void toDialLayout();

    void toSliderVLayout();

    void toSliderHLayout();

    void toTextBasedLayout();

    void disconnectDial();

    void connectDial();

private slots:
    void onDialChanged();

    void minLabelChangedReaction(double newVal);

    void maxLabelChangedReaction(double newVal);

    void valueLabelChangedReaction();
};

} /// namespace intelli

#endif // GT_INTELLI_DOUBLEINPUTWIDGET_H
