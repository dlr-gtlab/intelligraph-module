/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/lineobject.h>
#include <intelli/gui/style.h>

#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

using namespace intelli;

LineGraphicsObject::LineGraphicsObject(QGraphicsObject const& start,
                                       QGraphicsObject const* end) :
    m_startItem(&start),
    m_endItem(end ? end : &start)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    setZValue(style::zValue(style::ZValue::Line));

    setEndPoint(PortType::In, start);
    setEndPoint(PortType::Out, end ? *end : start);

    for (QGraphicsObject const* item : {m_startItem, m_endItem})
    {
        connect(item, &QGraphicsObject::xChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QGraphicsObject::yChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QGraphicsObject::widthChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QObject::destroyed, this, [this](){ delete this; });
    }
}

QRectF
LineGraphicsObject::boundingRect() const
{
    QRectF rect = QRectF{m_start, m_end}.normalized();
    return rect;
}

QPainterPath
LineGraphicsObject::shape() const
{
    QPainterPath path(m_start);
    path.lineTo(m_end);

    QPainterPathStroker stroker;
    stroker.setWidth(10.0);

    return stroker.createStroke(path);
}

QGraphicsObject const*
LineGraphicsObject::startItem() const
{
    return m_startItem;
}

QGraphicsObject const*
LineGraphicsObject::endItem() const
{
    return m_endItem;
}

void
LineGraphicsObject::setEndPoint(PortType type, QGraphicsObject const& object)
{
    setEndPoint(type, object.boundingRect().center() + object.pos());
}

void
LineGraphicsObject::setEndPoint(PortType type, QPointF pos)
{
    prepareGeometryChange();
    (type == PortType::In ? m_start : m_end) = pos;
}

void
LineGraphicsObject::updateEndPoints()
{
    setEndPoint(PortType::In, *m_startItem);
    setEndPoint(PortType::Out, *m_endItem);
}

void
LineGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    auto& style = style::currentStyle().connection;

    Qt::PenStyle penStyle = Qt::DotLine;
    QColor outColor = Qt::gray;
    QBrush penBrush = outColor;
    double penWidth = 0.5 * style.defaultOutlineWidth;

    QPen pen{penBrush, penWidth, penStyle};
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    // draw path
    QPainterPath path(m_start);
    path.lineTo(m_end);

    painter->drawPath(path);
}

void
LineGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = true;
    update();
    event->accept();
}

void
LineGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = false;
    update();
    event->accept();
}

void
LineGraphicsObject::updateEndPoint()
{
    auto* sender = this->sender();
    if (!sender) return;

    prepareGeometryChange();

    bool isStartItem = sender == (QObject*)&*m_startItem;

    assert(dynamic_cast<QGraphicsObject*>(sender));

    setEndPoint(isStartItem ? PortType::In : PortType::Out, *(QGraphicsObject*)sender);
}

