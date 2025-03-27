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

namespace intelli
{

class DoubleInputWidget : public AbstractNumberInputWidget
{
    Q_OBJECT
public:

    Q_INVOKABLE DoubleInputWidget(InputMode mode,
                                  QWidget* parent = nullptr);

    using AbstractNumberInputWidget::value;

    /// getter
    double value() const { return AbstractNumberInputWidget::value<double>(); }

    /// current min bound (may not be enforced, when not using bounds)
    double minV() const;
    /// current max bound (may not be enforced, when not using bounds)
    double maxV() const;
    /// resultion of slider
    int ticks() const;

protected slots:

    void applyRange(QVariant const& valueV,
                    QVariant const& minV,
                    QVariant const& maxV) override;

    void commitSliderValueChange(int value) override;

    void commitMinValueChange() override;

    void commitMaxValueChange() override;

    void commitValueChange() override;

private:

    double m_min = 0;
    double m_max = 0;
    const int m_ticks  = 1000;
};

} /// namespace intelli

#endif // GT_INTELLI_DOUBLEINPUTWIDGET_H
