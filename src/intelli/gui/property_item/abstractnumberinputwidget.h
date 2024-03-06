/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 05.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef ABSTRACTNUMBERINPUTWIDGET_H
#define ABSTRACTNUMBERINPUTWIDGET_H

#include <QWidget>
#include <QLayout>

namespace intelli
{
class AbstractNumberInputWidget : public QWidget
{
    Q_OBJECT

public:

    enum InputType
    {
        Dial = 0,
        SliderV = 1,
        SliderH = 2,
        LineEdit = 4
    };

    /**
     * @brief eventFilter
     * Defines an eventfilter to enable interaction witht he mouse in the
     * plot. Else the mouse action would be applied to the node
     * @param obj
     * @param e
     * @return
     */
    bool eventFilter(QObject* obj, QEvent* e) override;
signals:
    void sizeChanged();

protected:
    AbstractNumberInputWidget(QWidget* parent = nullptr);

    AbstractNumberInputWidget::InputType typeFromString(
            const QString& typeString);

    QLayout* newDialLayout(QWidget* slider, QWidget* minText,
                           QWidget* valueText, QWidget* maxText);

    QLayout* newSliderHLayout(QWidget* slider, QWidget* minText,
                              QWidget* valueText, QWidget* maxText);

    QLayout* newSliderVLayout(QWidget* slider, QWidget* minText,
                              QWidget* valueText, QWidget* maxText);

    void resizeEvent(QResizeEvent* event) override;
};
}
#endif // ABSTRACTNUMBERINPUTWIDGET_H
