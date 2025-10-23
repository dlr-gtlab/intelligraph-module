/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLDISPLAYGRAPHICSWIDGET_H
#define GT_INTELLI_BOOLDISPLAYGRAPHICSWIDGET_H

#include <QGraphicsWidget>

namespace intelli
{

class BoolDisplayGraphicsWidget : public QGraphicsWidget
{
    Q_OBJECT

public:

    enum DisplayMode
    {
        Checkbox = 0,
        Button
    };
    Q_ENUM(DisplayMode);

    explicit BoolDisplayGraphicsWidget(bool value = false, DisplayMode mode = DisplayMode::Button);

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

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void paint(QPainter* painter, QStyleOptionGraphicsItem const*, QWidget*) override;

private:

    DisplayMode m_mode{Checkbox};

    void applyDisplayMode(DisplayMode mode);

    bool m_value = false, m_readOnly = false, m_pressed = false;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLDISPLAYGRAPHICSWIDGET_H
