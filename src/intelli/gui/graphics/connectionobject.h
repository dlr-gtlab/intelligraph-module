/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H
#define GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H

#include <intelli/connection.h>

#include <QGraphicsObject>
#include <QPointer>

namespace intelli
{

class ConnectionGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    using ControlPoints = std::pair<QPointF, QPointF>;

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Connection };
    int type() const override { return Type; }

    ConnectionGraphicsObject(ConnectionId connection);

    QRectF boundingRect() const override;

    ConnectionId connectionId() const;

    QPointF endPoint(PortType type) const;
    void setEndPoint(PortType type, QPointF pos);

    ControlPoints pointsC1C2() const;

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QPainterPath shape() const override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    ConnectionId m_connection;
    mutable QPointF m_out;
    mutable QPointF m_in;

    bool m_hovered = false;

    QPainterPath cubicPath() const;
};

} // namespace intelli

#endif // CONNECTIONGRAPHICSOBJECT_H
