/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/ui/logicnodeui.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/style.h>
#include <intelli/node/logicoperation.h>

#include <gt_colors.h>

#include <QPainter>

using namespace intelli;

LogicNodeGeometry::LogicNodeGeometry(NodeGraphicsObject const& object) :
    NodeGeometry(object)
{
    assert(qobject_cast<Node const*>(&node()));
}

QRectF
LogicNodeGeometry::captionRect() const
{
    auto& style = style::currentStyle().node;

    QRectF inner = nodeBodyRect();
    QRectF rect = NodeGeometry::captionRect();
    return rect;
    rect.moveTo({inner.topLeft().x() + style.evalStateSize, -20});
    return rect;
}

QRect
LogicNodeGeometry::iconRect() const
{
    QRect rect = NodeGeometry::iconRect();
    return rect;
    QRectF captionRect = NodeGeometry::captionRect();
    rect.moveTopLeft((captionRect.topRight() - QPointF{0, (rect.height() - captionRect.height()) * 0.5})
                         .toPoint());
    return rect;
}

QRectF
LogicNodeGeometry::evalStateRect() const
{
    auto& style = style::currentStyle().node;

    QRectF rect = NodeGeometry::evalStateRect();
    QRectF caption = captionRect();
    double offset  = (rect.height() - caption.height()) * 0.5;
    rect.moveTo(caption.topLeft() - QPointF{(double)style.evalStateSize, offset});
    return rect;
}

QRectF
LogicNodeGeometry::portRect(PortType type, PortIndex idx) const
{
    QPointF p{};

    auto& style = style::currentStyle().node;

    size_t n = node().ports(type).size();

    switch (type)
    {
    case PortType::In:
    {
        assert(n >= 1);

        QPainterPath path = beginCurve();
        applyLeftCurve(path);

        double percentage = 1.0 / (double)(n + 1);
        p = path.pointAtPercent(percentage + (double)idx * percentage);

        if (logicNode().operation() == LogicNode::XOR)
        {
            p -= QPointF{0.5 * style.portRadius, 0};
        }
        break;
    }
    case PortType::Out:
    {
        assert(n == 1);

        QRectF rect = nodeBodyRect();
        p = rect.topRight() + QPointF{0, rect.height() * 0.5};
        p -= QPointF{style.portRadius, 0};
        break;
    }
    case PortType::NoType:
        // failing assert for debug builds
        assert(type != PortType::NoType);
        break;
    }

    p -= QPointF{style.portRadius, style.portRadius};

    return {
        p, QSizeF{style.portRadius * 2, style.portRadius * 2}
    };
}

QPainterPath
LogicNodeGeometry::beginCurve() const
{
    QPainterPath path(nodeBodyRect().topLeft());
    return path;
}

void
LogicNodeGeometry::applyLeftCurve(QPainterPath& path) const
{
    QRectF rect = nodeBodyRect();
    QPointF end = rect.bottomLeft();

    QPointF xOffset{0.25 * rect.width(), 0};

    if (path.currentPosition() == rect.bottomLeft()) end = rect.topLeft();

    switch (logicNode().operation())
    {
    case LogicNode::NOT:
    case LogicNode::NAND:
    case LogicNode::AND:
        path.lineTo(end);
        break;
    default:
        path.quadTo(rect.center()- xOffset, end);
        break;
    }
}

void
LogicNodeGeometry::applyRightCurve(QPainterPath& path) const
{
    QRectF rect = nodeBodyRect();
    QPointF start = path.currentPosition();
    QPointF end = rect.topLeft();
    QPointF yOffset{0, 0.5 * rect.height()};
    QPointF xOffset{0.25 * rect.width(), 0};
    QPointF midPos = rect.bottomLeft() - yOffset + xOffset;
    QPointF rightPos = rect.bottomRight() - yOffset;

    if (start == rect.topLeft())
    {
        end = rect.bottomLeft();
        yOffset *= -1;
    }

    switch (logicNode().operation())
    {
    case LogicNode::NOT:
        path.lineTo(rightPos);
        path.lineTo(end);
        break;
    case LogicNode::NAND:
    case LogicNode::NOR:
        // add cirle at output to denote inversion
        rightPos -= xOffset * 0.5;
        path.addEllipse(rightPos + xOffset * 0.25, xOffset.x() * 0.25, xOffset.x() * 0.25);
        path.moveTo(start);
    default:
        path.lineTo(midPos + yOffset);
        path.quadTo(rightPos - xOffset + yOffset, rightPos);
        path.quadTo(rightPos - xOffset - yOffset, midPos - yOffset);
        path.lineTo(end);
        break;
    }
}

LogicNode const&
LogicNodeGeometry::logicNode() const
{
    assert (qobject_cast<LogicNode const*>(&node()));
    return static_cast<LogicNode const&>(node());
}

QPainterPath
LogicNodeGeometry::computeShape() const
{
    QPainterPath path = beginCurve();
    applyLeftCurve(path);
    applyRightCurve(path);
    path = path.simplified();
    path.addRect(nodeHeaderRect());

    for (PortType type : {PortType::In, PortType::Out})
    {
        size_t size = node().ports(type).size();
        for (PortIndex idx{0}; idx < size; ++idx)
        {
            path.addRect(portRect(type, idx));
        }
    }
    return path;
}

QRectF
LogicNodeGeometry::computeNodeHeaderRect() const
{
    QRectF rect = NodeGeometry::computeNodeHeaderRect();
    return rect;
}

QRectF
LogicNodeGeometry::computeNodeBodyRect() const
{
    auto& node = this->node();
    auto& style = style::currentStyle().node;

    QRectF header = nodeHeaderRect();

    // height
    QFontMetrics metrcis(style.bodyFont);
    int offset = metrcis.height() * 0.6;
    int height = vspacing() + style.portRadius;

    size_t n = node.ports(PortType::In).size();
    for (PortIndex i{0}; i < n; ++i)
    {
        height += 2 * offset + vspacing();
    }

    // width
    int width = 50;

    switch (logicNode().operation())
    {
    case LogicNode::NOT:
        width *= 0.5;
        break;
    case LogicNode::NAND:
    case LogicNode::NOR:
        width += 0.25 * width;
    default:
        break;
    }

    // center body
    QPointF xOffset = QPointF{(header.width() - width) * 0.5, 0};

    return QRectF{
        header.bottomLeft() + xOffset,
        QSize{width, height}
    };
}

QRectF
LogicNodeGeometry::computeBoundingRect() const
{
    auto& style = style::currentStyle().node;

    QRectF upper = evalStateRect().united(captionRect());
    upper.setHeight(upper.height() + 20);
    QRectF lower = shape().boundingRect().translated(-style.portRadius, 0);
    lower.setWidth(lower.width() + 2 * style.portRadius);
    return upper.united(lower);
}

LogicNodePainter::LogicNodePainter(NodeGraphicsObject const& object,
                                   NodeGeometry const& geometry) :
    NodePainter(object, geometry)
{

}

void
LogicNodePainter::drawBackground(QPainter& painter) const
{
    if (object().isCollpased()) return NodePainter::drawBackground(painter);

    auto geo = static_cast<LogicNodeGeometry const*>(&geometry());

    applyBackgroundConfig(painter);

    QPainterPath path(geo->nodeBodyRect().topLeft());
    geo->applyLeftCurve(path);
    geo->applyRightCurve(path);
    painter.drawPath(path);
}

void
LogicNodePainter::drawOutline(QPainter& painter) const
{
    auto geo = static_cast<LogicNodeGeometry const*>(&geometry());

    applyOutlineConfig(painter);

    QPainterPath path(geo->nodeBodyRect().topLeft());
    geo->applyLeftCurve(path);

    if (static_cast<LogicNode const&>(object().node()).operation() == LogicNode::XOR)
    {
        auto& style = style::currentStyle().node;
        painter.drawPath(path.translated(-style.portRadius, 0));
    }

    geo->applyRightCurve(path);
    painter.drawPath(path);
}

void
LogicNodePainter::drawPortCaption(QPainter& painter,
                                  PortInfo const& port,
                                  PortType type,
                                  PortIndex idx,
                                  uint flags) const
{
    Q_UNUSED(painter);
    Q_UNUSED(port);
    Q_UNUSED(type);
    Q_UNUSED(idx);
    Q_UNUSED(flags);

    // not drawing caption due to size constraint
}

void
LogicNodePainter::drawPort(QPainter& painter,
                           PortInfo const& port,
                           PortType type,
                           PortIndex idx,
                           uint flags) const
{
    Q_UNUSED(painter);
    Q_UNUSED(port);
    Q_UNUSED(type);
    Q_UNUSED(idx);
    Q_UNUSED(flags);

    switch (type)
    {
    case PortType::Out:
    {
        applyPortConfig(painter, port, type, idx, flags);
        QPen pen = painter.pen();
        pen.setColor(painter.brush().color());
        painter.setBrush(Qt::NoBrush);
        painter.setPen(pen);

        bool isPortIncompatible =  (flags & HighlightPorts) &&
                                  !(flags & PortHighlighted);

        QSizeF offset = QSizeF{1, 1};
        if (isPortIncompatible) offset *= 3;

        QRectF p = geometry().portRect(type, idx);
        p.translate(offset.width() * 0.5, offset.height() * 0.5);
        p.setSize(p.size() - offset);

        painter.drawEllipse(p);
        break;
    }
    default:
        return NodePainter::drawPort(painter, port, type, idx, flags);
    }
}

LogicNodeUI::LogicNodeUI() = default;

std::unique_ptr<intelli::NodePainter>
intelli::LogicNodeUI::painter(NodeGraphicsObject const& object,
                              NodeGeometry const& geometry) const
{
    if (!qobject_cast<LogicNode const*>(&object.node()))
    {
        return NodeUI::painter(object, geometry);
    }

    assert(dynamic_cast<LogicNodeGeometry const*>(&geometry));
    return std::make_unique<LogicNodePainter>(object, geometry);
}

std::unique_ptr<NodeGeometry>
LogicNodeUI::geometry(NodeGraphicsObject const& object) const
{
    if (!qobject_cast<LogicNode const*>(&object.node()))
    {
        return NodeUI::geometry(object);
    }

    return std::make_unique<LogicNodeGeometry>(object);
}
