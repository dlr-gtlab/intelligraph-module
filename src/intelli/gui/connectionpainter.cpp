/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/connectionpainter.h>
#include <intelli/gui/connectiongeometry.h>

using namespace intelli;

ConnectionPainter::ConnectionPainter() = default;

void
ConnectionPainter::applyPenConfig(QPainter& painter,
                                  style::StyleData::ConnectionData const& cstyle,
                                  QColor const& startColor,
                                  QColor const& endColor,
                                  QPointF start,
                                  QPointF end,
                                  PainterFlags flags) const
{
    QBrush penBrush = startColor;

    Qt::PenStyle penStyle = Qt::SolidLine;
    double penWidth = cstyle.defaultOutlineWidth;

    if (flags & ObjectIsInactive)
    {
        penBrush = cstyle.inactiveOutline;
    }
    else if (flags & ObjectIsHovered)
    {
        penWidth = cstyle.hoveredOutlineWidth;
        penBrush = cstyle.hoveredOutline;
    }
    else if (flags & ObjectIsSelected)
    {
        penWidth = cstyle.selectedOutlineWidth;
        penBrush = cstyle.selectedOutline;
    }
    else if (flags & DrawGradient)
    {
        QLinearGradient gradient(start, end);
        gradient.setColorAt(0.3, startColor);
        gradient.setColorAt(0.7, endColor);
        penBrush = gradient;
    }

    if (flags & DrawDashed) penStyle = Qt::DashLine;
    else if (flags & DrawDotted) penStyle = Qt::DotLine;

    QPen pen{penBrush, penWidth, penStyle};
    painter.setPen(pen);
}

void
ConnectionPainter::drawPath(QPainter& painter,
                            QPainterPath const& path,
                            style::StyleData::ConnectionData const& cstyle,
                            TypeId const& startType,
                            TypeId const& endType,
                            PainterFlags flags) const
{
    QColor startColor = cstyle.typeColor(startType);
    QColor endColor   = cstyle.typeColor(endType);

    if (startType != endType) flags |= DrawGradient;

    drawPath(painter, path, cstyle, startColor, endColor, flags);
}

void
ConnectionPainter::drawPath(QPainter& painter,
                            QPainterPath const& path,
                            style::StyleData::ConnectionData const& cstyle,
                            QColor const& startColor,
                            QColor const& endColor,
                            PainterFlags flags) const
{
    QPointF start = path.pointAtPercent(0);
    QPointF end = path.pointAtPercent(1);

    applyPenConfig(painter, cstyle, startColor, endColor, start, end, flags);

    painter.drawPath(path);

    if (flags & ObjectIsSelected)
    {
        flags &= ~ObjectIsSelected;

        applyPenConfig(painter, cstyle, startColor, endColor, start, end, flags);

        painter.drawPath(path);
    }
}

void
ConnectionPainter::drawPath(QPainter& painter,
                            QPainterPath const& path,
                            style::StyleData::ConnectionData const& cstyle,
                            PainterFlags flags) const
{
    drawPath(painter, path, cstyle, cstyle.defaultOutline, cstyle.defaultOutline, flags);
}

void
ConnectionPainter::drawEndPoint(QPainter& painter,
                                QPainterPath const& path,
                                double radius,
                                PortType type) const
{
    painter.setBrush(painter.pen().brush());
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(path.pointAtPercent(type == PortType::Out ? 0.0 : 1.0), radius, radius);
}
