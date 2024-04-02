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

NodePainter::NodePainter(NodeGraphicsObject& obj, NodeGeometry& geometry) :
    m_object(&obj), m_geometry(&geometry)
{

}

QColor
NodePainter::backgroundColor() const
{
    auto& node = m_object->node();

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
NodePainter::drawBackground(QPainter& painter)
{
    // draw backgrond
    QColor color = backgroundColor();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    auto rect = m_geometry->innerRect();

    painter.drawRoundedRect(rect,
                            style::nodeRoundingRadius(),
                            style::nodeRoundingRadius());
}

void
NodePainter::drawOutline(QPainter& painter)
{
    auto rect = m_geometry->innerRect();

    bool selected = m_object->isSelected();
    bool hovered  = m_object->isHovered();

    QColor color = style::nodeOutline();
    double penWidth = style::nodeOutlineWidth();

    if (hovered)
    {
        penWidth = style::nodeHoveredOutlineWidth();
        color = style::nodeHoveredOutline();
    }
    if (selected)
    {
        color = style::nodeSelectedOutline();
    }

    QPen pen(color, penWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawRoundedRect(rect,
                            style::nodeRoundingRadius(),
                            style::nodeRoundingRadius());
}

void
NodePainter::drawPorts(QPainter& painter)
{
    auto& node = m_object->node();
    auto& graph = m_object->graph();

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
            painter.drawRect(m_geometry->portCaptionRect(type, idx));
            painter.drawRect(m_geometry->portRect(type, idx));
#endif
        }
    }
}

void
NodePainter::drawPort(QPainter& painter, Node::PortData& port, PortType type, PortIndex idx, bool connected)
{
    Q_UNUSED(connected);

    QRectF p = m_geometry->portRect(type, idx);

    //            double r = 1.0;

    //            if (auto const *cgo = state.connectionForReaction())
    //            {
    //                PortType requiredPort = cgo->connectionState().requiredPort();

    //                if (requiredPort == portType) {
    //                    ConnectionId possibleConnectionId = makeCompleteConnectionId(cgo->connectionId(),
    //                                                                                 nodeId,
    //                                                                                 portIndex);

    //                    bool const possible = model.connectionPossible(possibleConnectionId);

    //                    auto cp = cgo->sceneTransform().map(cgo->endPoint(requiredPort));
    //                    cp = ngo.sceneTransform().inverted().map(cp);

    //                    auto diff = cp - p;
    //                    double dist = std::sqrt(QPointF::dotProduct(diff, diff));

    //                    if (possible) {
    //                        double const thres = 40.0;
    //                        r = (dist < thres) ? (2.0 - dist / thres) : 1.0;
    //                    } else {
    //                        double const thres = 80.0;
    //                        r = (dist < thres) ? (dist / thres) : 1.0;
    //                    }
    //                }
    //            }

    double penWidth = m_object->isHovered() ?
                          style::nodeHoveredOutlineWidth() :
                          style::nodeOutlineWidth();

    QColor penColor = m_object->isSelected() ?
                          style::nodeSelectedOutline() :
                          style::nodeOutline();

    QBrush brush = style::connectionOutline(port.typeId);

    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawEllipse(p);

}

void
NodePainter::drawPortCaption(QPainter& painter, Node::PortData& port, PortType type, PortIndex idx, bool connected)
{
    auto& factory = NodeDataFactory::instance();

    painter.setPen(connected ? gt::gui::color::text() : gt::gui::color::disabled());

    painter.drawText(m_geometry->portCaptionRect(type, idx),
                     port.caption.isEmpty() ? factory.typeName(port.typeId) : port.caption,
                     type == PortType::In ? QTextOption{Qt::AlignLeft} : QTextOption{Qt::AlignRight});
}

void
NodePainter::drawResizeHandle(QPainter& painter)
{
    if (!m_object->centralWidget() ||
        !(m_object->node().nodeFlags() & NodeFlag::Resizable)) return;

    QRectF rect = m_geometry->resizeHandleRect();

    QPolygonF poly;
    poly.append(rect.bottomLeft());
    poly.append(rect.bottomRight());
    poly.append(rect.topRight());

    painter.setPen(Qt::NoPen);
    painter.setBrush(gt::gui::color::lighten(style::nodeOutline(), -30));
    painter.drawPolygon(poly);
}

void
NodePainter::drawCaption(QPainter& painter)
{
    auto& node = m_object->node();

    if (node.nodeFlags() & NodeFlag::HideCaption) return;

    QFont f = painter.font();
    bool isBold = f.bold();
    f.setBold(true);

    QRectF rect = m_geometry->captionRect();

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
NodePainter::paint(QPainter& painter)
{
    drawBackground(painter);
    drawResizeHandle(painter);
    drawOutline(painter);
    drawCaption(painter);
    drawPorts(painter);

#ifdef GT_INTELLI_DEBUG_GRAPHICS
    painter.setBrush(Qt::NoBrush);

    painter.setPen(Qt::red);
    painter.drawRect(m_object->boundingRect());

    painter.setPen(Qt::magenta);
    painter.drawPath(m_object->shape());

    painter.setPen(Qt::white);
    painter.drawRect(m_geometry->evalStateRect());
#endif
}
