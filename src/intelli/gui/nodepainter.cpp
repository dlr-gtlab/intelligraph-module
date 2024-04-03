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

NodePainter::NodePainter(NodeGraphicsObject& object, NodeGeometry& geometry) :
    m_object(&object), m_geometry(&geometry)
{

}

void
NodePainter::applyBackgroundConfig(QPainter& painter) const
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor());
}

void
NodePainter::applyOutlineConfig(QPainter& painter) const
{
    QColor penColor = style::nodeOutline();
    double penWidth = style::nodeOutlineWidth();

    if (object().isHovered())
    {
        penWidth = style::nodeHoveredOutlineWidth();
        penColor = style::nodeHoveredOutline();
    }
    if (object().isSelected())
    {
        penColor = style::nodeSelectedOutline();
    }

    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

QColor
NodePainter::backgroundColor() const
{
    auto& node = object().node();

    QColor const& bg = style::nodeBackground();

    if (node.nodeFlags() & NodeFlag::Unique)
    {
        return gt::gui::color::lighten(bg, -20);
    }
    if (qobject_cast<Graph*>(&node))
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

    painter.drawRoundedRect(rect,
                            style::nodeRoundingRadius(),
                            style::nodeRoundingRadius());
}

void
NodePainter::drawOutline(QPainter& painter) const
{
    applyOutlineConfig(painter);

    auto rect = geometry().innerRect();

    painter.drawRoundedRect(rect,
                            style::nodeRoundingRadius(),
                            style::nodeRoundingRadius());
}

void
NodePainter::drawPorts(QPainter& painter) const
{
    auto& node  = this->node();
    auto& graph = object().graph();

    for (PortType type : {PortType::Out, PortType::In})
    {
        size_t const n = node.ports(type).size();

        for (PortIndex idx{0}; idx < n; ++idx)
        {
            auto* port = node.port(node.portId(type, idx));
            assert(port);

            if (!port->visible) continue;

            bool connected = !graph.findConnections(node.id(), port->id()).empty();

            drawPort(painter, *port, type, idx, connected);

            if (!port->captionVisible) continue;

            drawPortCaption(painter, *port, type, idx, connected);

#ifdef GT_INTELLI_DEBUG_GRAPHICS
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
                      PortData& port,
                      PortType type,
                      PortIndex idx,
                      bool connected) const
{
    Q_UNUSED(connected);

    QRectF p = geometry().portRect(type, idx);

    double penWidth = object().isHovered() ?
                          style::nodeHoveredOutlineWidth() :
                          style::nodeOutlineWidth();

    QColor penColor = object().isSelected() ?
                          style::nodeSelectedOutline() :
                          style::nodeOutline();
    
    QBrush brush = style::typeIdColor(port.typeId);

    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    painter.setBrush(brush);

    painter.drawEllipse(p);

}

void
NodePainter::drawPortCaption(QPainter& painter,
                             PortData& port,
                             PortType type,
                             PortIndex idx,
                             bool connected) const
{
    auto& factory = NodeDataFactory::instance();

    painter.setBrush(Qt::NoBrush);
    painter.setPen(connected ? gt::gui::color::text() : gt::gui::color::disabled());

    painter.drawText(geometry().portCaptionRect(type, idx),
                     port.caption.isEmpty() ? factory.typeName(port.typeId) : port.caption,
                     type == PortType::In ? QTextOption{Qt::AlignLeft} : QTextOption{Qt::AlignRight});
}

void
NodePainter::drawResizeHandle(QPainter& painter) const
{
    if (!object().centralWidget() ||
        !(node().nodeFlags() & NodeFlag::Resizable)) return;

    QRectF rect = geometry().resizeHandleRect();

    QPolygonF poly;
    poly.append(rect.bottomLeft());
    poly.append(rect.bottomRight());
    poly.append(rect.topRight());

    painter.setPen(Qt::NoPen);
    painter.setBrush(gt::gui::color::lighten(style::nodeOutline(), -30));
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

#ifdef GT_INTELLI_DEBUG_GRAPHICS
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

#ifdef GT_INTELLI_DEBUG_GRAPHICS
    painter.setBrush(Qt::NoBrush);

    painter.setPen(Qt::white);
    painter.drawRect(geometry().evalStateRect());

    painter.setPen(Qt::red);
    painter.drawRect(object().boundingRect());

    painter.setPen(Qt::magenta);
    painter.drawPath(object().shape());
#endif
}

NodeGraphicsObject&
NodePainter::object() const
{
    assert(m_object);
    return *m_object;
}

Node&
NodePainter::node() const
{
    return object().node();
}

NodeGeometry&
NodePainter::geometry() const
{
    assert(m_geometry);
    return *m_geometry;
}
