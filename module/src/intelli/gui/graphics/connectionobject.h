/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H
#define GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H

#include <intelli/exports.h>
#include <intelli/connection.h>

#include <QGraphicsObject>
#include <QPointer>

namespace intelli
{

/**
 * @brief Graphics object used to represent a connection between to an output
 * and input port of two different nodes. Does not update the connection
 * automatically, this must be triggered by the graphics scene. It is possible
 * to apply different shapes to the connection.
 * The `pos` of this object is not representative of its actual position.
 */
class GT_INTELLI_EXPORT ConnectionGraphicsObject : public QGraphicsObject
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

    /// Control points for rectangle and cubic shapes
    using ControlPoints = std::pair<QPointF, QPointF>;

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Connection };
    int type() const override { return Type; }

    /**
     * @brief constructor
     * @param connection ConnectionId to render. May be partially invalid,
     * indicating a draft connection.
     * @param outType typeId of the output side, used for rendering
     * @param inType typeId of the input side, used for rendering
     */
    explicit ConnectionGraphicsObject(ConnectionId connection,
                                      TypeId outType = {},
                                      TypeId inType = {});

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

    /**
     * @brief The connection id this object refers to
     * @return Connection id
     */
    ConnectionId connectionId() const;

    /**
     * @brief Returns the corresponding end point of the connection
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @return End/start point respectively
     */
    QPointF endPoint(PortType type) const;

    /**
     * @brief Setter for the corresponding end point of this connection.
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @param pos New position of the end/start point
     */
    void setEndPoint(PortType type, QPointF pos);

    /**
     * @brief Sets the type id of the end/start point
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @param typeId New type id for the end/start point
     */
    void setPortTypeId(PortType type, TypeId typeId);

    /**
     * @brief Setter for the connection shape
     * @param shape
     */
    void setConnectionShape(ConnectionShape shape);

    /**
     * @brief Returns the control points to draw the connection shape properly.
     * For a straight connection these are the start and end point respectively.
     * @return Control points
     */
    ControlPoints controlPoints() const;

    /**
     * @brief Deemphasizes this object, i.e. to visually highlight other
     * objects.
     * @param inactive Whether this object should be made inactive
     */
    void makeInactive(bool inactive = true);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    /// Connection id
    ConnectionId m_connection;
    /// Type ids for the start and end point, used for rendering
    TypeId m_startType, m_endType;
    /// The shape of the connection
    ConnectionShape m_shape = ConnectionShape::DefaultShape;
    /// Start and end point
    mutable QPointF m_start, m_end;
    /// Whether the object is hovered
    bool m_hovered = false;
    /// Whether this object is considered inactive
    bool m_inactive = false;

    /// returns the painter path for the current connection shape.
    QPainterPath path() const;
};

} // namespace intelli

#endif // CONNECTIONGRAPHICSOBJECT_H
