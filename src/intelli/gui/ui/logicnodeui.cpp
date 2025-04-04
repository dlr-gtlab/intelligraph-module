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
    rect.moveTo({inner.topLeft().x() + style.evalStateSize, -20});
    return rect;
}

QRect
LogicNodeGeometry::iconRect() const
{
    QRect rect = NodeGeometry::iconRect();
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
    QPainterPath path = beginCurve();

    size_t n = 1;

    switch (type)
    {
    case PortType::In:
    {
        n = std::max(node().ports(type).size(), n);
        applyLeftCurve(path);
        break;
    }
    case PortType::Out:
    {
        n = std::max(node().ports(type).size(), n);
        applyRightCurve(path);
        break;
    }
    case PortType::NoType:
        // failing assert for debug builds
        assert(type != PortType::NoType);
        break;
    }

    auto& style = style::currentStyle().node;

    double percentage = 1.0 / (double)(n + 1);
    QPointF p = path.pointAtPercent(percentage + (double)idx * percentage);
    p -= QPointF{style.portRadius, style.portRadius};

    if (type == PortType::In && logicNode().operation() == LogicNode::XOR)
    {
        p -= QPointF{0.5 * style.portRadius, 0};
    }

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
    if (path.currentPosition() == rect.bottomLeft()) end = rect.topLeft();

    switch (logicNode().operation())
    {
    case LogicNode::NOT:
    case LogicNode::NAND:
    case LogicNode::AND:
        path.lineTo(end);
        break;
    default:
        path.quadTo(rect.center()- QPointF{0.25 * rect.width(), 0}, end);
        break;
    }
}

void
LogicNodeGeometry::applyRightCurve(QPainterPath& path) const
{
    QRectF rect = nodeBodyRect();
    QPointF end = rect.topLeft();
    QPointF rightPos = rect.center();
    QPointF offset{0, 0.5 * rect.height()};

    if (path.currentPosition() == rect.topLeft())
    {
        end = rect.bottomLeft();
        offset *= -1;
    }

    switch (logicNode().operation())
    {
    case LogicNode::NOT:
        rightPos += QPointF{0.5 * rect.width() - style::currentStyle().node.portRadius, 0};
        path.lineTo(rightPos);
        path.lineTo(end);
        break;
    default:
        rightPos += QPointF{0.5 * rect.width(), 0};
        path.cubicTo(rightPos + offset, rightPos - offset, end);
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
    path.addRect(captionRect().united(evalStateRect()).united(iconRect()));
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
LogicNodeGeometry::computeNodeBodyRect() const
{
    int n = node().ports(PortType::In).size() + 1;
    int height = n * (NodeGeometry::portRect(PortType::In, PortIndex{1}).topLeft().y() -
                      NodeGeometry::portRect(PortType::In, PortIndex{0}).topLeft().y());

    bool isNot = logicNode().operation() == LogicNode::NOT;
    int mult  = isNot ? 5 : 10;
    int width = mult * hspacing();
    return QRectF{QPoint{isNot ? 2 * hspacing() : 0, 0}, QSize{width, height}};
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
