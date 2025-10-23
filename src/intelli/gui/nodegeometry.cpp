/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/nodegeometry.h"

#include "intelli/nodedatafactory.h"
#include "intelli/node.h"
#include "intelli/gui/nodeuidata.h"
#include "intelli/gui/style.h"
#include "intelli/gui/graphics/nodeobject.h"

#include <QGraphicsWidget>

using namespace intelli;

NodeGeometry::NodeGeometry(NodeGraphicsObject const& object) :
    m_object(&object)
{

}

NodeGeometry::~NodeGeometry() = default;

void
NodeGeometry::setWidget(QPointer<QGraphicsWidget const> widget)
{
    m_widget = widget;
}

bool
NodeGeometry::positionWidgetBelowPorts() const
{
    return node().nodeFlags() & NodeFlag::MaximizeWidget;
}

int
NodeGeometry::hspacing() const
{
    return 10;
}

int
NodeGeometry::vspacing() const
{
    return 0.5 * hspacing();
}

bool
NodeGeometry::hasDisplayIcon() const
{
    return uiData().hasDisplayIcon() || object().isCollapsed();
}

int
NodeGeometry::portHorizontalExtent(PortType type) const
{
    int advance = 0;

    auto n = node().ports(type).size();
    for (PortIndex idx{0}; idx < n; ++idx)
    {
        advance = std::max(advance, (int)portCaptionRect(type, idx).width());
    }
    return advance + hspacing();
}

int
NodeGeometry::portVerticalExtent() const
{
    int height = vspacing();

    for (PortType type : {PortType::In, PortType::Out})
    {
        auto n = node().ports(type).size();
        if (n == 0) continue;

        for (PortIndex idx{0}; idx < n; ++idx)
        {
            height = std::max(height, (int)(portCaptionRect(type, idx).bottomLeft().y()));
        }
    }

    return height + vspacing();
}

QPainterPath const&
NodeGeometry::shape() const
{
    if (m_shape.has_value()) return *m_shape;

    // set empty value to avoid cyclic calls
    m_shape = QPainterPath{};
    m_shape = computeShape();
    return *m_shape;
}

QPainterPath
NodeGeometry::computeShape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

QRectF
NodeGeometry::nodeHeaderRect() const
{
    if (m_headerRect.has_value()) return *m_headerRect;

    // set empty value to avoid cyclic calls
    m_headerRect = QRectF{};
    m_headerRect = computeNodeHeaderRect();
    return *m_headerRect;
}

QRectF
NodeGeometry::computeNodeHeaderRect() const
{
    int height = 2 * vspacing() + captionSize().height();
    height = std::max(height, std::max(evalStateSize().height(),
                                       iconSize().height()));

    int bodyWidth =
        portHorizontalExtent(PortType::In) +
        portHorizontalExtent(PortType::Out) +
        hspacing(); // spacing between port captions and widget

    bodyWidth = positionWidgetBelowPorts() ?
                std::max(bodyWidth, widgetSize().width() + hspacing()) :
                bodyWidth + widgetSize().width();

    int headerWidth =
        hspacing() + // spacing between eval state and icon
        captionSize().width() +
        evalStateRect().width() +
        iconRect().width();

    int width = std::max(bodyWidth, headerWidth);

    m_bodyRect.reset();

    return QRectF{QPoint{0, 0}, QSize{width, height}};
}

QRectF
NodeGeometry::nodeBodyRect() const
{
    if (object().isCollapsed()) return nodeHeaderRect();

    if (m_bodyRect.has_value()) return *m_bodyRect;

    // set empty value to avoid cyclic calls
    m_bodyRect = QRectF{};
    m_bodyRect = computeNodeBodyRect();
    return *m_bodyRect;
}

QRectF
NodeGeometry::computeNodeBodyRect() const
{
    QRectF header = nodeHeaderRect();

    int width  = header.width();
    int height = portVerticalExtent();

    height = positionWidgetBelowPorts() ?
                 height + widgetSize().height() + vspacing() :
                 std::max(height, 2 * vspacing() + widgetSize().height());

    return QRectF{header.bottomLeft(), QSize{width, height}};
}

QRectF
NodeGeometry::boundingRect() const
{
    if (m_boundingRect.has_value()) return *m_boundingRect;

    // set empty value to avoid cyclic calls
    m_boundingRect = QRectF{};
    m_boundingRect = computeBoundingRect();

    // apply extra margin
    m_boundingRect->setSize(m_boundingRect->size() + QSizeF{2.0, 2.0});
    m_boundingRect->translate(-1.0, -1.0);
    return *m_boundingRect;
}

QRectF
NodeGeometry::computeBoundingRect() const
{
    auto& style = style::currentStyle().node;
    double xoffset = 1.0 * style.portRadius + 1;
    double yoffset = 0.5 * vspacing() + 1;

    auto rect = nodeBodyRect().united(nodeHeaderRect());

    return QRectF(rect.topLeft() - QPointF{xoffset, yoffset},
                  QSizeF{rect.width()  + 2 * xoffset,
                         rect.height() + 2 * yoffset});
}

QRectF
NodeGeometry::captionRect() const
{
    QRectF headerRect = this->nodeHeaderRect();
    QRectF caption{headerRect.topLeft(), captionSize()};

    // center caption
    double margin = headerRect.width() - caption.width();
    double xoffset = 0.5 * margin;

    // make the caption as centered as possible
    assert(margin);
    xoffset += (evalStateSize().width() - iconSize().width()) * 0.5;

    return caption.translated(xoffset, vspacing());
}

QSize
NodeGeometry::captionSize() const
{
    auto& style = style::currentStyle().node;
    QFontMetrics metrics{style.headerFont};

    constexpr int errorMargin = 2; // margin to avoid truncation of caption

    int width = metrics.horizontalAdvance(node().caption());
    width += (width & 1) + errorMargin;

    return QSize{width, metrics.height()};
}

QRect
NodeGeometry::iconRect() const
{
    constexpr QPoint padding = {-2, 2};

    return QRect{
        nodeHeaderRect().topRight().toPoint() - QPoint{iconSize().width(), 0} + padding,
        iconSize()
    };
}

QSize
NodeGeometry::iconSize() const
{
    if (!hasDisplayIcon()) return QSize{0, 0};

    auto& style = style::currentStyle().node;
    return QSize{style.iconSize, style.iconSize};
}

QRectF
NodeGeometry::evalStateRect() const
{
    return QRectF{
        nodeHeaderRect().topLeft(),
        evalStateSize()
    };
}

QSize
NodeGeometry::evalStateSize() const
{
    auto& style = style::currentStyle().node;
    return QSize{style.evalStateSize, style.evalStateSize};
}

QPointF
NodeGeometry::widgetPosition() const
{
    if (!centralWidget() || object().isCollapsed()) return {};

    auto body = nodeBodyRect();

    if (positionWidgetBelowPorts())
    {
        double xOffset = 0.5 * (body.width() - widgetSize().width());
        double yOffset = portVerticalExtent();

        return QPointF{xOffset, yOffset};
    }

    // node inbetween port captions
    int portsDiff = portHorizontalExtent(PortType::Out) -
                    portHorizontalExtent(PortType::In);

    double xOffset = (body.width() * 0.5) -
                     (portsDiff * 0.5) -
                     (widgetSize().width() * 0.5);

    double yOffset = vspacing();

    return body.topLeft() + QPointF{xOffset, yOffset};
}

QSize
NodeGeometry::widgetSize() const
{
    auto* w = centralWidget();
    if (!w || object().isCollapsed()) return {};
    return w->size().toSize();
}

QRectF
NodeGeometry::portRect(PortType type, PortIndex idx) const
{
    // bounds check
    assert(type != PortType::NoType);
    if (node().ports(type).size() < idx) return {};

    auto& node = this->node();
    auto& style = style::currentStyle().node;

    auto body = nodeBodyRect();

    // width
    double width = type == PortType::Out ? body.width() : 0.0;
    width -= style.portRadius;

    if (object().isCollapsed())
    {
        // position port at vertical center if collapsed
        int height = body.height() * 0.5 - style.portRadius;

        return QRectF{
            QPointF(width, height),
            QSizeF{style.portRadius * 2, style.portRadius * 2}
        };
    }

    // height
    QFontMetrics metrcis(style.bodyFont);
    int offset = metrcis.height() * 0.6;
    int height = body.topLeft().y() + vspacing() + style.portRadius;

    // height of all ports before
    for (PortIndex i{0}; i < idx; ++i)
    {
        auto* port = node.port(node.portId(type, i));
        assert(port);

        bool visible = port->visible;
        if (!visible) continue;

        height += 2 * offset + vspacing();
    }

    return QRectF{
        QPointF(width, height),
        QSizeF{style.portRadius * 2, style.portRadius * 2}
    };
}

QRectF
NodeGeometry::portCaptionRect(PortType type, PortIndex idx) const
{
    // bounds check
    assert(type != PortType::NoType);
    if (node().ports(type).size() < idx) return {};

    if (object().isCollapsed()) return {};

    auto& style = style::currentStyle().node;
    auto& node  = this->node();
    auto* port  = node.port(node.portId(type, idx));
    assert(port);

    if (!port->visible) return {};

    // height
    QFontMetrics metrics{style.bodyFont};
    int height = metrics.height();

    // width
    int width = 0;
    if (port->captionVisible)
    {
        auto& factory = NodeDataFactory::instance();

        width += metrics.horizontalAdvance(port->caption.isEmpty() ?
                                               factory.typeName(port->typeId) :
                                               port->caption);
        width += (width & 1);
    }

    // position
    QPointF pos = portRect(type, idx).center();
    pos.setY(pos.y() - height * 0.5);
    pos.setX(type == PortType::In ?
                 pos.x() + hspacing() :
                 pos.x() - hspacing() - width);

    return QRectF{pos, QSize{width, height}};
}

NodeGeometry::PortHit
NodeGeometry::portHit(QPointF coord) const
{
    constexpr QPointF offset{0.5, 0.5};
    return portHit(QRectF{coord - offset, coord + offset});
}

NodeGeometry::PortHit
NodeGeometry::portHit(QRectF rect) const
{
    if (object().isCollapsed()) return {};

    auto body  = nodeBodyRect();
    auto coord = rect.center();

    // estimate whether its a input or output port
    PortType type = (coord.x() < (body.x() + 0.5 * body.width())) ?
                        PortType::In : PortType::Out;

    auto& node = this->node();

    // check each port
    for (auto& port : node.ports(type))
    {
        if (!port.visible) continue;

        auto pRect = this->portRect(type, node.portIndex(type, port.id()));
        if (pRect.intersects(rect))
        {
            return PortHit{type, port.id()};
        }
    }

    return {};
}

QRectF
NodeGeometry::resizeHandleRect() const
{
    constexpr QSize size{8, 8};

    QRectF body = nodeBodyRect();
    return QRectF(body.bottomRight() - QPoint{size.width(), size.height()}, size);
}

void
NodeGeometry::recomputeGeometry()
{
    m_shape.reset();
    m_boundingRect.reset();
    m_bodyRect.reset();
    m_headerRect.reset();
}

NodeUIData const&
NodeGeometry::uiData() const
{
    return object().uiData();
}

NodeGraphicsObject const&
NodeGeometry::object() const
{
    assert(m_object);
    return *m_object;
}

Node const&
NodeGeometry::node() const
{
    return object().node();
}

QGraphicsWidget const*
NodeGeometry::centralWidget() const
{
    return m_widget;
}

