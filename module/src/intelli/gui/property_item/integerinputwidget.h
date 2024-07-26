/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef INTEGERINPUTWIDGET_H
#define INTEGERINPUTWIDGET_H

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

    void sliderReleased();

    void onMinLabelChanged(int newVal);

    void onMaxLabelChanged(int newVal);

    void onValueLabelChanged(int newVal);

public slots:
    void onMinMaxPropertiesChanged(int val, int min, int max);

    void onSliderTypeChanged(QString const& t);

private:
    int m_maxTicks;

    QAbstractSlider* m_dial;

    GtLineEdit* m_text;

    EditableIntegerLabel* m_low;

    EditableIntegerLabel* m_high;

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
    void onDialChanged(int newDialVal);

    void minLabelChangedReaction(int newVal);

    void maxLabelChangedReaction(int newVal);

    void valueLabelChangedReaction();
};
} // namespace intelli
#endif // INTEGERINPUTWIDGET_H
