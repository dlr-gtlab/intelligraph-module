/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/connectionobject.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/connectionpainter.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/graph.h>
#include <intelli/node.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

constexpr QPointF s_connection_distance{5, 5};

using namespace intelli;

ConnectionGraphicsObject::ConnectionGraphicsObject(QGraphicsScene& scene,
                                                   Connection* object,
                                                   ConnectionId connection,
                                                   NodeGraphicsObject const* outNodeObj,
                                                   NodeGraphicsObject const* inNodeObj) :
    m_object(object),
    m_outNode(outNodeObj),
    m_inNode(inNodeObj),
    m_connection(connection)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    // object ptr should be valid if the connection is valid
    assert((!!m_object) == !isDraft());
    // if connection is draft only node should be valid
    assert((!!m_outNode ^ !!m_inNode) == isDraft());

    scene.addItem(this);

    if (isDraft()) grabMouse();

    setZValue(style::zValue(isDraft() ?
                                style::ZValue::DraftConnection :
                                style::ZValue::Connection));

    connect(this, &GraphicsObject::hoveredChanged, this, [this](){
        return setZValue(style::zValue(isHovered() ?
                                           style::ZValue::ConnectionHovered :
                                           style::ZValue::Connection));
    });

    // update type ids if port changes to make sure connections stay updated
    for (auto pair : {std::make_pair(outNodeObj, PortType::Out),
                      std::make_pair(inNodeObj , PortType::In )})
    {
        NodeGraphicsObject const* nodeObj = std::get<0>(pair);
        if (!nodeObj) continue;

        Node const* node = &nodeObj->node();
        PortType const type = std::get<1>(pair);
        PortId const portId = connectionId().port(type);
        assert(portId.isValid());

        // update connection data if port changes
        auto updatePortType = [this, node, type, portId](PortId id){
            if (portId != id) return;
            auto* port = node->port(id);
            assert(port);
            setPortTypeId(type, port->typeId);
        };

        connect(node, &Node::portChanged, this, updatePortType);
        updatePortType(connectionId().port(type));

        // update connection's position if node object changes
        auto updateEndPoint = [this, nodeObj, type, portId](){
            QPointF endPoint = calcEndPoint(nodeObj, type, portId);
            setEndPoint(type, endPoint);
        };
        connect(nodeObj, &NodeGraphicsObject::xChanged, this, updateEndPoint);
        connect(nodeObj, &NodeGraphicsObject::yChanged, this, updateEndPoint);
        connect(nodeObj, &NodeGraphicsObject::nodeGeometryChanged, this, updateEndPoint);
        updateEndPoint();
    }
}

std::unique_ptr<ConnectionGraphicsObject>
ConnectionGraphicsObject::makeConnection(QGraphicsScene& scene,
                                         Connection& object,
                                         NodeGraphicsObject const& outNodeObj,
                                         NodeGraphicsObject const& inNodeObj)
{
    auto obj = new ConnectionGraphicsObject(
        scene, &object, object.connectionId(), &outNodeObj, &inNodeObj
    );
    return std::unique_ptr<ConnectionGraphicsObject>{obj};
}

std::unique_ptr<ConnectionGraphicsObject>
ConnectionGraphicsObject::makeDraftConnection(QGraphicsScene& scene,
                                              ConnectionId draftConId,
                                              NodeGraphicsObject const& startObj)
{
    PortType type = draftConId.draftType();
    auto obj = new ConnectionGraphicsObject(
        scene, nullptr, draftConId,
        type == PortType::Out ? &startObj : nullptr,
        type == PortType::Out ? nullptr   : &startObj
    );
    return std::unique_ptr<ConnectionGraphicsObject>{obj};
}

ConnectionGraphicsObject::~ConnectionGraphicsObject() = default;


bool
ConnectionGraphicsObject::deleteObject()
{
    if (isDraft()) return false;
    delete m_object;
    return true;
}

bool
ConnectionGraphicsObject::isDraft() const
{
    return connectionId().isDraft();
}

Connection*
ConnectionGraphicsObject::connection()
{
    assert(m_object && !isDraft());
    return m_object;
}

Connection const*
ConnectionGraphicsObject::connection() const
{
    return const_cast<ConnectionGraphicsObject*>(this)->connection();
}

QRectF
ConnectionGraphicsObject::boundingRect() const
{
    return m_geometry.boundingRect();
}

QPainterPath
ConnectionGraphicsObject::shape() const
{
    return m_geometry.shape();
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

    m_geometry.recomputeGeometry(m_start, m_end, m_shape);
    update();
}

void
ConnectionGraphicsObject::setPortTypeId(PortType type, TypeId typeId)
{
    (type == PortType::In ? m_inType : m_outType) = std::move(typeId);

    update();
}

void
ConnectionGraphicsObject::setConnectionShape(ConnectionShape shape)
{
    if (m_shape == shape) return;

    prepareGeometryChange();
    m_shape = shape;
    m_geometry.recomputeGeometry(m_start, m_end, m_shape);
    update();
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

    bool const isDraft  = this->isDraft();
    bool const isInactive = m_inactive;

    PortType const draftType = connectionId().draftType();
    assert(!isDraft == (draftType == PortType::NoType)); // NoType if not draft

    ConnectionPainter::PainterFlags flags = 0;
    if (isHovered())  flags |= ConnectionPainter::ObjectIsHovered;
    if (isSelected()) flags |= ConnectionPainter::ObjectIsSelected;
    if (isDraft)      flags |= ConnectionPainter::DrawDashed;
    if (isInactive)   flags |= ConnectionPainter::ObjectIsInactive;

    auto const& style = style::currentStyle();
    auto const& cstyle = style.connection;

    auto const path = m_geometry.path();

    ConnectionPainter p;
    p.drawPath(
        *painter,
        path,
        cstyle,
        draftType == PortType::In  ? m_inType  : m_outType,
        draftType == PortType::Out ? m_outType : m_inType,
        flags
    );

    if (isDraft)
    {
        double const portRadius = style.node.portRadius;
        p.drawEndPoint(*painter, path, portRadius, invert(draftType));
    }

#ifdef GT_INTELLI_DEBUG_CONNECTION_GRAPHICS
// (adapted)
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    QPointF in  = endPoint(PortType::In);
    QPointF out = endPoint(PortType::Out);

    auto const points = m_geometry.controlPoints(in, out, m_shape);

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
// SPDX-SnippetEnd
#endif
}

QVariant
ConnectionGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
    {
        bool isSelected = value.toBool();
        setZValue(style::zValue(!isSelected ?
                                    style::ZValue::Connection :
                                    style::ZValue::ConnectionHovered));
        break;
    }
    default:
        break;
    }

    return value;
}

void
ConnectionGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!isDraft()) return GraphicsObject::mouseMoveEvent(event);

    assert(scene());

    // snap to nearest possible port
    ConnectionId conId = connectionId();
    PortType draftType = conId.draftType();

    QPointF pos = event->scenePos();
    QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

    auto const& items = scene()->items(rect);
    for (auto* item : items)
    {
        auto* object = graphics_cast<NodeGraphicsObject*>(item);
        if (!object) continue;

        auto hit = object->geometry().portHit(object->mapFromScene(rect).boundingRect());
        if (!hit) continue;

        bool reverse = draftType == PortType::In;
        if (reverse) conId.reverse();

        conId.inNodeId = object->nodeId();
        conId.inPort   = hit.port;
        assert(conId.isValid());

        if (reverse) conId.reverse();

        auto* graph = Graph::accessGraph(object->node());
        if (!graph || !graph->canAppendConnections(conId)) continue;

        QPointF endPoint = calcEndPoint(object, hit.type, hit.port);
        setEndPoint(invert(draftType), endPoint);
        return event->accept();
    }

    setEndPoint(invert(draftType), event->scenePos());

    return GraphicsObject::mouseMoveEvent(event);
}

void
ConnectionGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (!isDraft()) return GraphicsObject::mouseMoveEvent(event);

    assert(scene());

    ungrabMouse();

    ConnectionId conId = connectionId();
    PortType draftType = conId.draftType();

    QPointF pos = event->scenePos();

    QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

    // find node to connect to
    auto const& items = scene()->items(rect);
    for (auto* item : items)
    {
        auto* object = qgraphicsitem_cast<NodeGraphicsObject*>(item);
        if (!object) continue;

        auto hit = object->geometry().portHit(object->mapFromScene(rect).boundingRect());
        if (!hit) continue;

        bool reverse = draftType == PortType::In;
        if (reverse) conId.reverse();

        conId.inNodeId = object->nodeId();
        conId.inPort   = hit.port;
        assert(conId.isValid());

        if (reverse) conId.reverse();

        auto* graph = Graph::accessGraph(object->node());
        if (!graph || !graph->canAppendConnections(conId)) continue;

        event->accept();

        break;
    }

    event->accept();
    emit finalizeDraftConnnection(conId);
    delete this;
}

QPointF
ConnectionGraphicsObject::calcEndPoint(NodeGraphicsObject const* nodeObj,
                                       PortType portType,
                                       PortId portId)
{
    assert(nodeObj);

    PortIndex const portIdx = nodeObj->node().portIndex(portType, portId);
    assert(portIdx.isValid());
    auto const& geometry = nodeObj->geometry();

    QRectF const portRect = geometry.portRect(portType, portIdx);
    QPointF const nodePos = nodeObj->sceneTransform().map(portRect.center());

    return sceneTransform().inverted().map(nodePos);
}
