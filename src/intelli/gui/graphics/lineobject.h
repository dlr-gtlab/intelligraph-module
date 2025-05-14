/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LINEGRAPHICSOBJECT_H
#define GT_INTELLI_LINEGRAPHICSOBJECT_H

#include <intelli/globals.h>

#include <QGraphicsObject>
#include <QPointer>

namespace intelli
{

class LineGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Line };
    int type() const override { return Type; }

    LineGraphicsObject(QGraphicsObject const& start,
                       QGraphicsObject const* end = nullptr);

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

    /**
     * @brief Shape used for collision detection
     * @return Shape
     */
    QPainterPath shape() const override;

    QGraphicsObject const* startItem() const;

    QGraphicsObject const* endItem() const;

    void setEndPoint(PortType type, QGraphicsObject const& object);

    void setEndPoint(PortType type, QPointF pos);

public slots:

    void updateEndPoints();

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    QPointer<QGraphicsObject const> m_startItem, m_endItem;
    /// Start and end point
    QPointF m_start, m_end;
    /// Whether the object is hovered
    bool m_hovered = false;

private slots:

    void updateEndPoint();
};

} // namespace intelli

#endif // GT_INTELLI_LINEGRAPHICSOBJECT_H
