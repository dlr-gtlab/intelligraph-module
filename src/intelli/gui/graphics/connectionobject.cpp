/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/graphics/connectionobject.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/style.h>

#include <gt_colors.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>

using namespace intelli;

ConnectionGraphicsObject::ConnectionGraphicsObject(ConnectionId connection) :
    m_connection(connection)
{
//    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    setZValue(-1.0);
}

QRectF
ConnectionGraphicsObject::boundingRect() const
{
    auto points = controlPoints();

    // `normalized()` fixes inverted rects.
    QRectF basicRect = QRectF{m_out, m_in}.normalized();

    QRectF c1c2Rect = QRectF(points.first, points.second).normalized();

    QRectF commonRect = basicRect.united(c1c2Rect);

    // TODO: define in style
    constexpr float const diam = 8;
    QPointF const cornerOffset(diam, diam);

    // Expand rect by port circle diameter
    commonRect.setTopLeft(commonRect.topLeft() - cornerOffset);
    commonRect.setBottomRight(commonRect.bottomRight() + 2 * cornerOffset);

    return commonRect;
}

ConnectionId
ConnectionGraphicsObject::connectionId() const
{
    return m_connection;
}

QPointF
ConnectionGraphicsObject::endPoint(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        return m_in;
    case PortType::Out:
        return m_out;
    case PortType::NoType:
        break;
    }
    throw GTlabException(__FUNCTION__, "invalid port type!");
}

void
ConnectionGraphicsObject::setEndPoint(PortType type, QPointF pos)
{
    prepareGeometryChange();

    switch (type)
    {
    case PortType::In:
        m_in = pos;
        break;
    case PortType::Out:
        m_out = pos;
        break;
    case PortType::NoType:
    default:
        throw GTlabException(__FUNCTION__, "invalid port type!");
    }

    update();
}

void
ConnectionGraphicsObject::setConnectionShape(ConnectionShape shape)
{
    if (m_shape == shape) return;

    prepareGeometryChange();
    m_shape = shape;
    update();
}

ConnectionGraphicsObject::ControlPoints
ConnectionGraphicsObject::controlPoints() const
{
    switch (m_shape)
    {

    case ConnectionShape::Straight:
    {
        return {m_out, m_in};
    }

    case ConnectionShape::Cubic:
    {
        constexpr double maxControlPointExtent = 200;

        double xDistance = m_in.x() - m_out.x();

        double horizontalOffset = qMin(maxControlPointExtent, std::abs(xDistance)) * 0.5;

        double verticalOffset = 0;

        if (xDistance < 0)
        {
            constexpr double offset = 5;

            double yDistance = m_in.y() - m_out.y() + offset;

            verticalOffset  = qMin(maxControlPointExtent, std::abs(yDistance));
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset *= 2;
        }

        QPointF c1 = m_out + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = m_in  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
    }

    case ConnectionShape::Rectangle:
    {
        constexpr double cutoffAngle = 0.025;

        double xDistance = m_in.x() - m_out.x();
        double yDistance = m_in.y() - m_out.y();

        double horizontalOffset = std::abs(xDistance) * 0.5;

        double verticalOffset = 0;

        if (xDistance < 0)
        {
            constexpr double maxHorizontalOffset = 10;

            double yDistance = m_in.y() - m_out.y();

            verticalOffset  = std::abs(yDistance) * 0.5;
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset = qMin(maxHorizontalOffset, horizontalOffset);
        }
        else if (std::abs(yDistance / (xDistance + 0.1)) <= cutoffAngle)
        {
            return {m_out, m_in};
        }

        QPointF c1 = m_out + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = m_in  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
    }

    }

    return {m_out, m_in};
}

void
ConnectionGraphicsObject::paint(QPainter* painter,
                                QStyleOptionGraphicsItem const* option,
                                QWidget* widget)
{
    painter->setClipRect(option->exposedRect);

    bool const hovered  = m_hovered;
    bool const selected = isSelected();
    bool const isDraft  = !m_connection.isValid();

    // TODO: line width
    double lineWidth = 3.0;

    if (hovered || selected) lineWidth = 4.0;
    else if (isDraft) lineWidth = 4.0;

    // TODO: color
    QColor color = Qt::darkCyan;
    if (hovered) color = Qt::lightGray;
    else if (selected) color = style::boundarySelected();
    else if (isDraft) color = gt::gui::color::disabled();

    QPen pen;
    pen.setWidth(lineWidth);
    pen.setColor(color);

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    // cubic spline
    auto const path = this->path();
    painter->drawPath(path);

    // draw end points
    painter->setPen(Qt::gray);
    painter->setBrush(Qt::gray);
    // TODO: point radius
    double const pointRadius = 10.0 / 2.0;
    painter->drawEllipse(m_out, pointRadius, pointRadius);
    painter->drawEllipse(m_in,  pointRadius, pointRadius);

    constexpr bool debug = false;
    if (debug)
    {
        QPointF in  = endPoint(PortType::In);
        QPointF out = endPoint(PortType::Out);

        auto const points = controlPoints();

        painter->setPen(Qt::red);
        painter->setBrush(Qt::red);

        painter->drawLine(QLineF(out, points.first));
        painter->drawLine(QLineF(points.first, points.second));
        painter->drawLine(QLineF(points.second, in));
        painter->drawEllipse(points.first, 3, 3);
        painter->drawEllipse(points.second, 3, 3);

        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);

        painter->setPen(Qt::yellow);
        painter->drawRect(boundingRect());
    }
}

QPainterPath
ConnectionGraphicsObject::shape() const
{
    constexpr size_t segments = 20;

    auto path = this->path();

    QPainterPath result(endPoint(PortType::Out));

    for (size_t i = 0; i < segments; ++i)
    {
        double ratio = double(i + 1) / segments;
        result.lineTo(path.pointAtPercent(ratio));
    };

    QPainterPathStroker stroker;
    stroker.setWidth(10.0);

    return stroker.createStroke(result);
}

void
ConnectionGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsObject::mousePressEvent(event);
}

void
ConnectionGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsObject::mouseReleaseEvent(event);
}

void
ConnectionGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = true;
    update();
    event->accept();
}

void
ConnectionGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = false;
    update();
    event->accept();
}

QPainterPath
ConnectionGraphicsObject::path() const
{

    QPointF const& in = endPoint(PortType::In);
    QPointF const& out = endPoint(PortType::Out);

    auto const c1c2 = controlPoints();

    // cubic spline
    QPainterPath path(out);

    switch (m_shape)
    {
    case ConnectionShape::Cubic:
        path.cubicTo(c1c2.first, c1c2.second, in);
        break;
    case ConnectionShape::Rectangle:
        path.lineTo(c1c2.first);
        path.lineTo(c1c2.second);
    case ConnectionShape::Straight:
        path.lineTo(in);
        break;
    }

    return path;
}

