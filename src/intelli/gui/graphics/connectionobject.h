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

class ConnectionColorCache
{
public:

    static ConnectionColorCache& instance();

    QColor colorOfType(QString const& typeName);

private:

    ConnectionColorCache() = default;

    QHash<QString, QColor> m_colors;
};

class ConnectionGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    enum ConnectionShape
    {
        Cubic = 0,
        Straight,
        Rectangle,
        DefaultShape = Cubic
    };
    Q_ENUM(ConnectionShape);

    using ControlPoints = std::pair<QPointF, QPointF>;

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Connection };
    int type() const override { return Type; }

    ConnectionGraphicsObject(ConnectionId connection,
                             TypeId inType  = {},
                             TypeId outType = {});

    QRectF boundingRect() const override;

    ConnectionId connectionId() const;

    QPointF endPoint(PortType type) const;
    void setEndPoint(PortType type, QPointF pos);

    void setPortTypeId(PortType type, TypeId typeId);

    void setConnectionShape(ConnectionShape shape);

    ControlPoints controlPoints() const;

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
    TypeId m_outType, m_inType;
    ConnectionShape m_shape = ConnectionShape::DefaultShape;
    mutable QPointF m_out;
    mutable QPointF m_in;

    bool m_hovered = false;

    QPainterPath path() const;
};

} // namespace intelli

#endif // CONNECTIONGRAPHICSOBJECT_H
