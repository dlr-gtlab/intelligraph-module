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
#include "intelli/gui/style.h"

#include <QWidget>

using namespace intelli;

NodeGeometry::NodeGeometry(Node const& node) :
    m_node(&node)
{

}

NodeGeometry::~NodeGeometry() = default;

void
NodeGeometry::setWidget(QPointer<QWidget const> widget)
{
    m_widget = widget;
}

bool
NodeGeometry::positionWidgetAtBottom() const
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

int
NodeGeometry::captionHeightExtend() const
{
    QRectF caption = captionRect();
    return caption.height() + 2 * caption.topLeft().y();
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
    return advance;
}

int
NodeGeometry::portHeightExtent() const
{
    QRectF caption = captionRect();
    int height = caption.height() + caption.topLeft().y();

    for (PortType type : {PortType::In, PortType::Out})
    {
        auto n = node().ports(type).size();
        if (n == 0) continue;

        for (PortIndex idx{0}; idx < n; ++idx)
        {
            height = std::max(height, (int)(portCaptionRect(type, idx).bottomLeft().y() + vspacing()));
        }
    }

    return height;
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
NodeGeometry::innerRect() const
{
    if (m_innerRect.has_value()) return *m_innerRect;

    // set empty value to avoid cyclic calls
    m_innerRect = QRectF{};
    m_innerRect = computeInnerRect();
    return *m_innerRect;
}

QRectF
NodeGeometry::computeInnerRect() const
{
    QSize wSize{0, 0};
    if (auto w = centralWidget())
    {
        wSize = w->size();
    }
    bool positionAtBottom = positionWidgetAtBottom();

    // height
    int height = vspacing() + portHeightExtent();

    // width
    int width = 0;
    for (PortType type : {PortType::In, PortType::Out})
    {
        width += portHorizontalExtent(type) + hspacing();
    }

    if (positionAtBottom)
    {
        height += wSize.height();
        width  += hspacing();
        width   = std::max(width, (int)wSize.width() + hspacing());
    }
    else
    {
        height = std::max(height, captionHeightExtend() + wSize.height() + vspacing());
        width += hspacing() + wSize.width();
    }

    auto& style = style::currentStyle().node;
    width = std::max(width, (int)(style.evalStateSize + hspacing() + captionRect().width()));

    return QRectF(QPoint{0, 0}, QSize{width, height});
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

    auto rect = innerRect();

    return QRectF(rect.topLeft() - QPointF{xoffset, yoffset},
                  QSizeF{rect.width()  + 2 * xoffset,
                         rect.height() + 2 * yoffset});
}

QRectF
NodeGeometry::captionRect() const
{
    QFont f;
    f.setBold(true);
    QFontMetrics metrics{f};

    constexpr int errorMargin = 2;

    // center caption
    int width = metrics.horizontalAdvance(node().caption());
    width += (width & 1) + errorMargin;

    QRectF innerRect = this->innerRect();
    QRectF caption{innerRect.topLeft(), QSize{width, metrics.height()}};

    double margin = innerRect.width() - caption.width();

    double xoffset = 0.5 * (innerRect.width() - caption.width());

    // make the caption as centered as possible
    auto& style = style::currentStyle().node;
    xoffset += (style.evalStateSize / margin) * 0.5 * style.evalStateSize;

    return caption.translated(xoffset, vspacing());
}

QRectF
NodeGeometry::evalStateRect() const
{
    auto& style = style::currentStyle().node;
    return QRectF{
        innerRect().topLeft(),
        QSizeF{style.evalStateSize, style.evalStateSize}
    };
}

QPointF
NodeGeometry::widgetPosition() const
{
    auto* w = centralWidget();
    if (!w) return {};

    if (positionWidgetAtBottom())
    {
        double xOffset = 0.5 * (innerRect().width() - w->width());
        double yOffset = portHeightExtent();

        return QPointF{xOffset, yOffset};
    }


    int portsDiff = portHorizontalExtent(PortType::Out) -
                    portHorizontalExtent(PortType::In);

    double xOffset = (innerRect().width() * 0.5) -
                     (portsDiff * 0.5) -
                     (w->width() * 0.5);

    double yOffset = captionHeightExtend();

    return QPointF{xOffset, yOffset};
}

// TODO: what if port index is out of bounds?
QRectF
NodeGeometry::portRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    auto& node  = this->node();

    QFontMetrics metrics{QFont()};

    double height = captionHeightExtend() + vspacing();

    for (PortIndex i{0}; i < idx; ++i)
    {
        auto* port = node.port(node.portId(type, i));
        bool visible = !port || port->visible;

        height += visible * 1.5 * metrics.height();
    }

    auto& style = style::currentStyle().node;

    double width = type == PortType::Out ? innerRect().width() : 0.0;
    width -= style.portRadius;

    return {
        QPointF(width, height),
        QSizeF{style.portRadius * 2, style.portRadius * 2}
    };
}

QRectF
NodeGeometry::portCaptionRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    auto& node = this->node();
    auto* port = node.port(node.portId(type, idx));
    assert(port);

    if (!port->visible) return {};

    QFontMetrics metrics{QFont()};

    int height = metrics.height();
    int width = 0;

    if (port->captionVisible)
    {
        auto& factory = NodeDataFactory::instance();

        width += metrics.horizontalAdvance(port->caption.isEmpty() ?
                                               factory.typeName(port->typeId) :
                                               port->caption);
        width += (width & 1);
    }

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
    auto inner = innerRect();
    auto coord = rect.center();

    // estimate whether its a input or output port
    PortType type = (coord.x() < (inner.x() + 0.5 * inner.width())) ?
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

    QRectF rect = innerRect();
    return QRectF(rect.bottomRight() - QPoint{size.width(), size.height()}, size);
}

void
NodeGeometry::recomputeGeomtry()
{
    m_innerRect.reset();
    m_shape.reset();
    m_boundingRect.reset();
}

Node const&
NodeGeometry::node() const
{
    assert(m_node);
    return *m_node;
}

QWidget const*
NodeGeometry::centralWidget() const
{
    return m_widget;
}

