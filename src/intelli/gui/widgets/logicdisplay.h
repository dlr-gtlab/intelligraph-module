/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LOGICDISPLAY_H
#define GT_INTELLI_LOGICDISPLAY_H

#include <QLabel>

namespace intelli
{

class LogicDisplayWidget : public QWidget
{
    Q_OBJECT

public:

    explicit LogicDisplayWidget(QWidget* parent = nullptr) :
        LogicDisplayWidget(false, parent)
    {}

    explicit LogicDisplayWidget(bool value, QWidget* parent = nullptr);

    bool value() const;

    void setReadOnly(bool readOnly = true);

    bool readOnly() const;

public slots:

    void toogle();

    void setValue(bool value);

signals:

    void valueChanged(bool newValue);

protected:

    void mousePressEvent(QMouseEvent* e) override;

    void paintEvent(QPaintEvent* e) override;

private:

    bool m_value = false, m_readOnly = false;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICDISPLAY_H
