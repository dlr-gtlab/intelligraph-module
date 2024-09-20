/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H
#define GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H

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

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H
