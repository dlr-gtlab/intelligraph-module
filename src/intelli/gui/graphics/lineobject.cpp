/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/lineobject.h>
#include <intelli/gui/connectionpainter.h>
#include <intelli/gui/style.h>

#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>

using namespace intelli;

constexpr QPointF s_connection_distance{5, 5};

LineGraphicsObject::LineGraphicsObject(InteractableGraphicsObject const& start,
                                       InteractableGraphicsObject const* end) :
    m_startItem(&start),
    m_endItem(end)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    setZValue(style::zValue(style::ZValue::Line));

    setEndPoint(PortType::Out, start);
    setEndPoint(PortType::In, end ? *end : start);

    for (InteractableGraphicsObject const* item : {m_startItem, m_endItem})
    {
        if (!item) continue;
        connect(item, &QGraphicsObject::xChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QGraphicsObject::yChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QGraphicsObject::widthChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &QGraphicsObject::heightChanged,
                this, &LineGraphicsObject::updateEndPoint);
        connect(item, &InteractableGraphicsObject::objectResized,
                this, &LineGraphicsObject::updateEndPoints);
    }
}

std::unique_ptr<LineGraphicsObject>
LineGraphicsObject::makeLine(InteractableGraphicsObject const& startObj,
                             InteractableGraphicsObject const& endObj)
{
    auto obj = new LineGraphicsObject{startObj, &endObj};
    return std::unique_ptr<LineGraphicsObject>(obj);
}

std::unique_ptr<LineGraphicsObject>
LineGraphicsObject::makeDraftLine(InteractableGraphicsObject const& startObj)
{
    auto obj = new LineGraphicsObject{startObj, nullptr};
    return std::unique_ptr<LineGraphicsObject>(obj);
}

LineGraphicsObject::~LineGraphicsObject() = default;


bool
LineGraphicsObject::deleteObject()
{
    emit deleteRequested();
    return true;
}

bool
LineGraphicsObject::isDraft() const
{
    return !m_endItem;
}

void
LineGraphicsObject::setTypeMask(size_t mask)
{
    m_mask = mask;
}

QRectF
LineGraphicsObject::boundingRect() const
{
    return m_geometry.boundingRect();
}

QPainterPath
LineGraphicsObject::shape() const
{
    return m_geometry.shape();
}

GraphicsObject const*
LineGraphicsObject::startItem() const
{
    return m_startItem;
}

GraphicsObject const*
LineGraphicsObject::endItem() const
{
    return m_endItem;
}

void
LineGraphicsObject::setEndPoint(PortType type, QGraphicsItem const& object)
{
    setEndPoint(type, object.boundingRect().center() + object.pos());
}

void
LineGraphicsObject::setEndPoint(PortType type, QPointF pos)
{
    prepareGeometryChange();
    (type == PortType::Out ? m_start : m_end) = pos;
    m_geometry.recomputeGeometry(m_start, m_end, ConnectionShape::Straight);
}

void
LineGraphicsObject::updateEndPoints()
{
    assert(m_startItem);
    setEndPoint(PortType::Out, *m_startItem);
    if (m_endItem) setEndPoint(PortType::In, *m_endItem);
}

void
LineGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    auto style = style::currentStyle().connection;
    style.defaultOutline = style.hoveredOutline;
    style.defaultOutlineWidth = 1;
    style.hoveredOutlineWidth = 2;
    style.selectedOutlineWidth = 2;

    ConnectionPainter::PainterFlags flags = ConnectionPainter::DrawDotted;
    if (isHovered())  flags = ConnectionPainter::ObjectIsHovered;
    if (isSelected()) flags = ConnectionPainter::ObjectIsSelected;

    ConnectionPainter p;
    p.drawPath(*painter, m_geometry.path(), style, flags);
}

void
LineGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!isDraft()) return GraphicsObject::mousePressEvent(event);

    assert(scene());

    ungrabMouse();

    event->accept();

    QPointF pos = event->scenePos();
    QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

    auto const& items = scene()->items(rect);
    for (auto* item : items)
    {
        if (!item || ((size_t)item->type() & m_mask) != m_mask) continue;

        return emit finalizeDraftConnection(item);
    }

    emit finalizeDraftConnection(nullptr);
}

void
LineGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!isDraft()) return GraphicsObject::mouseMoveEvent(event);

    assert(scene());

    event->accept();

    QPointF pos = event->scenePos();
    QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

    auto const& items = scene()->items(rect);
    for (auto* item : items)
    {
        if (!item || ((size_t)item->type() & m_mask) != m_mask) continue;

        return setEndPoint(PortType::In, *item);
    }

    setEndPoint(PortType::In, event->scenePos());
}

void
LineGraphicsObject::updateEndPoint()
{
    auto* sender = this->sender();
    if (!sender) return;

    prepareGeometryChange();

    bool isStartItem = sender == (QObject*)&*m_startItem;

    assert(dynamic_cast<GraphicsObject*>(sender));

    setEndPoint(isStartItem ? PortType::Out : PortType::In, *(GraphicsObject*)sender);
}

