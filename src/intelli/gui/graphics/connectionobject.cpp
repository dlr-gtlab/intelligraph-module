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

ConnectionGraphicsObject::ConnectionGraphicsObject(ConnectionId connection,
                                                   TypeId outType,
                                                   TypeId inType) :
    m_connection(connection),
    m_startType(std::move(outType)),
    m_endType(std::move(inType))
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    setZValue(style::zValue(connection.isDraft() ? ZValue::DraftConnection :
                                                   ZValue::Connection));
}

QRectF
ConnectionGraphicsObject::boundingRect() const
{
    auto points = controlPoints();

    // `normalized()` fixes inverted rects.
    QRectF basicRect = QRectF{m_start, m_end}.normalized();

    QRectF c1c2Rect = QRectF(points.first, points.second).normalized();

    QRectF commonRect = basicRect.united(c1c2Rect);

    double const diam = style::nodePortSize() * 2;
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
        return m_end;
    case PortType::Out:
        return m_start;
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
        m_end = pos;
        break;
    case PortType::Out:
        m_start = pos;
        break;
    case PortType::NoType:
    default:
        throw GTlabException(__FUNCTION__, "invalid port type!");
    }

    update();
}

void
ConnectionGraphicsObject::setPortTypeId(PortType type, TypeId typeId)
{
    (type == PortType::In ? m_endType : m_startType) = std::move(typeId);

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
        return {m_start, m_end};
    }

    case ConnectionShape::Cubic:
    {
        constexpr double maxControlPointExtent = 200;

        double xDistance = m_end.x() - m_start.x();

        double horizontalOffset = qMin(maxControlPointExtent, std::abs(xDistance)) * 0.5;

        double verticalOffset = 0;

        if (xDistance < 0)
        {
            constexpr double offset = 5;

            double yDistance = m_end.y() - m_start.y() + offset;

            verticalOffset  = qMin(maxControlPointExtent, std::abs(yDistance));
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset *= 2;
        }

        QPointF c1 = m_start + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = m_end  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
    }

    case ConnectionShape::Rectangle:
    {
        constexpr double cutoffValue = 0.025;

        double xDistance = m_end.x() - m_start.x();
        double yDistance = m_end.y() - m_start.y();

        double horizontalOffset = std::abs(xDistance) * 0.5;

        double verticalOffset = 0;

        if (xDistance < 0)
        {
            constexpr double maxHorizontalOffset = 10;

            double yDistance = m_end.y() - m_start.y();

            verticalOffset  = std::abs(yDistance) * 0.5;
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset = qMin(maxHorizontalOffset, horizontalOffset);
        }
        // dont draw rectangle shaped connections if y distance is small
        else if (std::abs(yDistance / (xDistance + 0.1)) <= cutoffValue)
        {
            return {m_start, m_end};
        }

        QPointF c1 = m_start + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = m_end  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
    }

    }

    return {m_start, m_end};
}

void
ConnectionGraphicsObject::makeInactive(bool inactive)
{
    m_inactive = inactive;
    update();
}

void
ConnectionGraphicsObject::paint(QPainter* painter,
                                QStyleOptionGraphicsItem const* option,
                                QWidget* widget)
{
    Q_UNUSED(widget);

    painter->setClipRect(option->exposedRect);

    bool hovered  = m_hovered;
    bool selected = isSelected();
    bool isDraft  = m_connection.isDraft();
    bool isInactive = m_inactive;

    auto const makePen = [this, &hovered, &selected, &isDraft, &isInactive](){

        QColor outColor = style::typeIdColor(m_startType);

        double penWidth = style::connectionPathWidth();
        Qt::PenStyle penStyle = Qt::SolidLine;
        QBrush penBrush = outColor;

        if (isInactive)
        {
            penBrush = style::connectionInactivePath();
        }
        else if (hovered)
        {
            penWidth = style::connectionHoveredPathWidth();
            penBrush = style::connectionHoveredPath();
        }
        else if (selected)
        {
            penWidth = style::connectionHoveredPathWidth();
            penBrush = style::connectionSelectedPath();
        }
        else if (isDraft)
        {
            PortType draftType = m_connection.draftType();
            if (draftType == PortType::In)
            {
                penBrush = style::typeIdColor(m_endType);
            }

            penWidth = style::connectionDraftPathWidth();
            penStyle = Qt::DashLine;
        }
        else if (m_startType != m_endType)
        {
            QColor inColor  = style::typeIdColor(m_endType);
            QLinearGradient gradient(m_start, m_end);
            gradient.setColorAt(0.1, outColor);
            gradient.setColorAt(0.9, inColor);
            penBrush = gradient;
        }

        return QPen{penBrush, penWidth, penStyle};
    };

    QPen pen = makePen();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    // draw path
    auto const path = this->path();
    painter->drawPath(path);

    if (selected)
    {
        selected = false;
        pen = makePen();
        pen.setWidth(pen.width() - 1);
        painter->setPen(pen);
        painter->setPen(pen);
        painter->drawPath(path);
    }

    // draw end points
    if (isDraft)
    {
        double const pointRadius = style::nodePortSize();

        painter->setPen(Qt::NoPen);
        painter->setBrush(pen.brush());
        painter->drawEllipse(m_connection.draftType() == PortType::Out ? m_end : m_start,
                             pointRadius, pointRadius);
    }

#ifdef GT_INTELLI_DEBUG_CONNECTION_GRAPHICS
    QPointF in  = endPoint(PortType::In);
    QPointF out = endPoint(PortType::Out);

    auto const points = controlPoints();

    painter->setPen(Qt::magenta);
    painter->setBrush(Qt::magenta);

    painter->drawLine(QLineF(out, points.first));
    painter->drawLine(QLineF(points.first, points.second));
    painter->drawLine(QLineF(points.second, in));
    painter->drawEllipse(points.first, 3, 3);
    painter->drawEllipse(points.second, 3, 3);

    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

    painter->setPen(Qt::red);
    painter->drawRect(boundingRect());
#endif
}

QVariant
ConnectionGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
        setZValue(style::zValue(!value.toBool() ? ZValue::Connection : ZValue::ConnectionHovered));
        break;
    default:
        break;
    }

    return value;
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
    setZValue(style::zValue(ZValue::ConnectionHovered));
    update();
    event->accept();
}

void
ConnectionGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = false;
    setZValue(style::zValue(ZValue::Connection));
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
        path.lineTo(in);
        break;
    case ConnectionShape::Straight:
        path.lineTo(in);
        break;
    }

    return path;
}
