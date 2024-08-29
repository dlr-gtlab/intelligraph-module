/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef DOUBLEINPUTWIDGET_H
#define DOUBLEINPUTWIDGET_H

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

    void sliderReleased();

    void onMinLabelChanged(double newVal);

    void onMaxLabelChanged(double newVal);

    void onValueLabelChanged(double newVal);

public slots:
    void onMinMaxPropertiesChanged(double val, double min, double max);

    void onSliderTypeChanged(QString const& t);

private:
    int m_maxTicks;

    QAbstractSlider* m_dial;

    GtLineEdit* m_text;

    EditableDoubleLabel* m_low;

    EditableDoubleLabel* m_high;

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
    void onDialChanged(int newDialVal);

    void minLabelChangedReaction(double newVal);

    void maxLabelChangedReaction(double newVal);

    void valueLabelChangedReaction();
};

} /// namespace intelli
#endif // DOUBLEINPUTWIDGET_H
