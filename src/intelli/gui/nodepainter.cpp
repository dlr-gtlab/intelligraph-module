/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 26.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/nodepainter.h"
#include "intelli/gui/nodegeometry.h"
#include "intelli/gui/style.h"
#include "intelli/gui/graphics/nodeobject.h"

#include "intelli/nodedatafactory.h"
#include "intelli/graph.h"

#include <gt_colors.h>

#include <QPainter>

using namespace intelli;

NodePainter::NodePainter(NodeGraphicsObject const& object,
                         NodeGeometry const& geometry) :
    m_object(&object), m_geometry(&geometry)
{

}

void
NodePainter::applyBackgroundConfig(QPainter& painter) const
{
    QColor bg = backgroundColor();

    painter.setPen(Qt::NoPen);
    painter.setBrush(bg);
}

void
NodePainter::applyOutlineConfig(QPainter& painter) const
{
    auto& object = this->object();

    auto& style = style::currentStyle().node;
    QColor penColor = style.defaultOutline;
    double penWidth = style.defaultOutlineWidth;

    bool const selected = object.isSelected();
    bool const hovered = object.isHovered();

    if (selected)
    {
        penWidth = hovered ?
                       style.hoveredOutlineWidth :
                       style.selectedOutlineWidth ;
        penColor = style.selectedOutline;
    }
    else if (hovered)
    {
        penWidth = style.hoveredOutlineWidth;
        penColor = style.hoveredOutline;
    }

    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

QColor
NodePainter::backgroundColor() const
{
    auto& object = this->object();
    auto& highlights = object.highlights();

    // apply tint if node is compatible
    if (highlights.isActive())
    {
        auto& style = style::currentStyle().node;

        QColor bg = style.background;
        if (!highlights.isNodeCompatible()) return bg;

        int val = style.compatiblityTintModifier;
        return style::tint(bg, val);
    }

    auto bg = customBackgroundColor();
    return bg;
}


QColor
NodePainter::customBackgroundColor() const
{
    auto& node = this->node();

    QColor bg = style::currentStyle().node.background;

    if (node.nodeFlags() & NodeFlag::Unique)
    {
        return gt::gui::color::lighten(bg, -20);
    }
    if (qobject_cast<Graph const*>(&node))
    {
        return gt::gui::color::lighten(bg, -12);
    }

    return bg;
}

void
NodePainter::drawBackground(QPainter& painter) const
{
    applyBackgroundConfig(painter);

    auto rect = geometry().innerRect();

    auto& style = style::currentStyle().node;
    painter.drawRoundedRect(rect, style.roundingRadius, style.roundingRadius);
}

void
NodePainter::drawOutline(QPainter& painter) const
{
    applyOutlineConfig(painter);

    auto rect = geometry().innerRect();

    auto& style = style::currentStyle().node;
    painter.drawRoundedRect(rect, style.roundingRadius, style.roundingRadius);
}

void
NodePainter::drawPorts(QPainter& painter) const
{
    auto& node  = this->node();
    auto& object = this->object();
    auto& graph = object.graph();
    auto& highlights = object.highlights();

    for (PortType type : {PortType::Out, PortType::In})
    {
        size_t const n = node.ports(type).size();

        for (PortIndex idx{0}; idx < n; ++idx)
        {
            auto* port = node.port(node.portId(type, idx));
            assert(port);

            if (!port->visible) continue;
            
            auto& conModel = graph.connectionModel();
            auto* conData = connection_model::find(conModel, node.id());
            assert(conData);

            bool connected = connection_model::hasConnections(*conData, port->id());

            uint flags = NoPortFlag;

            if (connected) flags |= PortConnected;
            if (highlights.isActive())
            {
                flags |= HighlightPorts;

                bool highlighted = highlights.isPortCompatible(port->id());
                if (highlighted) flags |= PortHighlighted;
            }

            drawPort(painter, *port, type, idx, flags);

            if (!port->captionVisible) continue;

            drawPortCaption(painter, *port, type, idx, flags);

#ifdef GT_INTELLI_DEBUG_NODE_GRAPHICS
            painter.setPen(Qt::yellow);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(geometry().portCaptionRect(type, idx));
            painter.drawRect(geometry().portRect(type, idx));
#endif
        }
    }
}

void
NodePainter::drawPort(QPainter& painter,
                      PortInfo const& port,
                      PortType type,
                      PortIndex idx,
                      uint flags) const
{
    bool isPortIncompatible =  (flags & HighlightPorts) &&
                              !(flags & PortHighlighted);

    QSizeF offset = QSizeF{1, 1};
    if (isPortIncompatible) offset *= 3;

    auto& style = style::currentStyle();
    auto& nstyle = style.node;

    double penWidth = object().isHovered() ?
                          nstyle.hoveredOutlineWidth :
                          nstyle.defaultOutlineWidth;

    QColor penColor = object().isSelected() ?
                          nstyle.selectedOutline :
                          nstyle.defaultOutline;

    QBrush brush = isPortIncompatible ?
                       style.connection.inactiveOutline :
                       style.connection.typeColor(port.typeId);

    QRectF p = geometry().portRect(type, idx);
    p.translate(offset.width() * 0.5, offset.height() * 0.5);
    p.setSize(p.size() - offset);

    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    painter.setBrush(brush);

    painter.drawEllipse(p);
}

void
NodePainter::drawPortCaption(QPainter& painter,
                             PortInfo const& port,
                             PortType type,
                             PortIndex idx,
                             uint flags) const
{
    auto& factory = NodeDataFactory::instance();

    painter.setBrush(Qt::NoBrush);
    painter.setPen(flags & PortConnected ?
                       gt::gui::color::text() :
                       gt::gui::color::disabled());

    painter.drawText(geometry().portCaptionRect(type, idx),
                     port.caption.isEmpty() ? factory.typeName(port.typeId) : port.caption,
                     type == PortType::In ? QTextOption{Qt::AlignLeft} : QTextOption{Qt::AlignRight});
}

void
NodePainter::drawResizeHandle(QPainter& painter) const
{
    if (!object().hasResizeHandle()) return;

    QRectF rect = geometry().resizeHandleRect();

    QPolygonF poly;
    poly.append(rect.bottomLeft());
    poly.append(rect.bottomRight());
    poly.append(rect.topRight());

    painter.setPen(Qt::NoPen);
    painter.setBrush(gt::gui::color::lighten(
        style::currentStyle().node.defaultOutline, -30));
    painter.drawPolygon(poly);
}

void
NodePainter::drawCaption(QPainter& painter) const
{
    auto& node = this->node();

    if (node.nodeFlags() & NodeFlag::HideCaption) return;

    QFont f = painter.font();
    bool isBold = f.bold();
    f.setBold(true);

    QRectF rect = geometry().captionRect();

    painter.setFont(f);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(gt::gui::color::text());
    painter.drawText(rect, node.caption(), QTextOption{Qt::AlignHCenter});

    f.setBold(isBold);
    painter.setFont(f);

#ifdef GT_INTELLI_DEBUG_NODE_GRAPHICS
    painter.setBrush(Qt::NoBrush);

    painter.setPen(Qt::white);
    painter.drawRect(rect);
#endif
}

void
NodePainter::paint(QPainter& painter) const
{
    drawBackground(painter);
    drawResizeHandle(painter);
    drawOutline(painter);
    drawCaption(painter);
    drawPorts(painter);

#ifdef GT_INTELLI_DEBUG_NODE_GRAPHICS
    painter.setBrush(Qt::NoBrush);

    painter.setPen(Qt::white);
    painter.drawRect(geometry().evalStateRect());

    painter.setPen(Qt::red);
    painter.drawRect(object().boundingRect());

    painter.setPen(Qt::magenta);
    painter.drawPath(object().shape());
#endif
}

NodeGraphicsObject const&
NodePainter::object() const
{
    assert(m_object);
    return *m_object;
}

Node const&
NodePainter::node() const
{
    return object().node();
}

NodeGeometry const&
NodePainter::geometry() const
{
    assert(m_geometry);
    return *m_geometry;
}
