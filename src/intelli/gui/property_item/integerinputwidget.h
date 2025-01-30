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

namespace intelli
{

class IntegerInputWidget : public AbstractNumberInputWidget
{
    Q_OBJECT

public:

    Q_INVOKABLE IntegerInputWidget(InputMode mode, QWidget* parent = nullptr);

    using AbstractNumberInputWidget::value;

    int value() const { return AbstractNumberInputWidget::value<int>(); }

    int min() const;
    int max() const;

protected slots:

    void applyRange(QVariant const& valueV,
                    QVariant const& minV,
                    QVariant const& maxV) override;

    void commitSliderValueChange(int value) override;

    void commitMinValueChange() override;

    void commitMaxValueChange() override;

    void commitValueChange() override;

private:

    int m_min = 0;
    int m_max = 0;
};

} // namespace intelli

#endif // GT_INTELLI_INTEGERINPUTWIDGET_H
