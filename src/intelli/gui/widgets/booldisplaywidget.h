/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLDISPLAYWIDGET_H
#define GT_INTELLI_BOOLDISPLAYWIDGET_H

#include <QWidget>

namespace intelli
{

class BoolDisplayWidget : public QWidget
{
    Q_OBJECT

public:

    enum DisplayMode
    {
        Checkbox = 0,
        Button
    };
    Q_ENUM(DisplayMode);

    explicit BoolDisplayWidget(QWidget* parent = nullptr) :
        BoolDisplayWidget(false, DisplayMode::Button, parent)
    {}
    explicit BoolDisplayWidget(bool value, DisplayMode mode, QWidget* parent = nullptr);

    bool value();

    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode() const;

    void setReadOnly(bool readOnly = true);
    bool readOnly();

public slots:

    void toogle();

    void setValue(bool value);

signals:

    void valueChanged(bool newValue);

protected:

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private:

    DisplayMode m_mode{Checkbox};

    void applyDisplayMode(DisplayMode mode);

    bool m_value = false, m_readOnly = false, m_pressed = false;
};

} // namespace intelli

#endif // BOOLDISPLAYWIDGET_H
