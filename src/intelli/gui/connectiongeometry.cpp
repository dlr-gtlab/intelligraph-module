/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/connectiongeometry.h>
#include <intelli/gui/style.h>

using namespace intelli;


ConnectionGeometry::ConnectionGeometry() = default;

ConnectionGeometry::ControlPoints
ConnectionGeometry::controlPoints(QPointF start, QPointF end, ConnectionShape shape)
{
    switch (shape)
    {

    case ConnectionShape::Straight:
    {
        return {start, end};
    }

    case ConnectionShape::Cubic:
    {
        // (adapted)
        // SPDX-SnippetBegin
        // SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
        // SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
        constexpr double maxControlPointExtent = 200;

        double xDistance = end.x() - start.x();

        double horizontalOffset = qMin(maxControlPointExtent, std::abs(xDistance)) * 0.5;
        double verticalOffset   = 0;

        if (xDistance < 0)
        {
            constexpr double offset = 5;

            double yDistance = end.y() - start.y() + offset;

            verticalOffset  = qMin(maxControlPointExtent, std::abs(yDistance));
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset *= 2;
        }

        QPointF c1 = start + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = end  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
        // SPDX-SnippetEnd
    }

    case ConnectionShape::Rectangle:
    {
        constexpr double cutoffValue = 0.025;

        double xDistance = end.x() - start.x();
        double yDistance = end.y() - start.y();

        double horizontalOffset = std::abs(xDistance) * 0.5;
        double verticalOffset   = 0;

        if (xDistance < 0)
        {
            constexpr double maxHorizontalOffset = 10;

            yDistance = end.y() - start.y();

            verticalOffset  = std::abs(yDistance) * 0.5;
            verticalOffset *= (yDistance < 0) ? -1.0 : 1.0;

            horizontalOffset = qMin(maxHorizontalOffset, horizontalOffset);
        }
        // dont draw rectangle shaped connections if y distance is small
        else if (std::abs(yDistance / (xDistance + 0.1)) <= cutoffValue)
        {
            return {start, end};
        }

        QPointF c1 = start + QPointF{horizontalOffset, verticalOffset};
        QPointF c2 = end  - QPointF{horizontalOffset, verticalOffset};

        return {c1, c2};
    }

    }
}


void
ConnectionGeometry::recomputeGeometry(QPointF start, QPointF end, ConnectionShape shape)
{
    computePath(start, end, shape);

    computeShape(start, end, shape);

    computeBoundingRect(start, end, shape);
}

void
ConnectionGeometry::computePath(QPointF start, QPointF end, ConnectionShape shape)
{
    auto c1c2 = controlPoints(start, end, shape);

    // cubic spline
    QPainterPath path(start);

    switch (shape)
    {
    case ConnectionShape::Cubic:
        path.cubicTo(c1c2.first, c1c2.second, end);
        break;
    case ConnectionShape::Rectangle:
        path.lineTo(c1c2.first);
        path.lineTo(c1c2.second);
        path.lineTo(end);
        break;
    case ConnectionShape::Straight:
        path.lineTo(end);
        break;
    }

    m_path = path;
}

void
ConnectionGeometry::computeShape(QPointF start, QPointF end, ConnectionShape shape)
{
    QPainterPathStroker stroker;
    stroker.setWidth(2 * style::currentStyle().node.portRadius);
    m_shape = stroker.createStroke(m_path);
}

void
ConnectionGeometry::computeBoundingRect(QPointF start, QPointF end, ConnectionShape shape)
{
    // (adapted)
    // SPDX-SnippetBegin
    // SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
    // SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    auto points = controlPoints(start, end, shape);

    // `normalized()` fixes inverted rects.
    QRectF basicRect = QRectF{start, end}.normalized();

    QRectF c1c2Rect = QRectF(points.first, points.second).normalized();

    QRectF commonRect = basicRect.united(c1c2Rect);

    double const diam = style::currentStyle().node.portRadius * 2;
    QPointF const cornerOffset(diam, diam);

    // Expand rect by port circle diameter
    commonRect.setTopLeft(commonRect.topLeft() - cornerOffset);
    commonRect.setBottomRight(commonRect.bottomRight() + 2 * cornerOffset);

    m_rect = commonRect;
    // SPDX-SnippetEnd
}

