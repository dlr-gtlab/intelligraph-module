/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_CONNECTIONGEOMETRY_H
#define GT_INTELLI_CONNECTIONGEOMETRY_H

#include <intelli/globals.h>
#include <intelli/gui/style.h>

#include <QPainterPath>

namespace intelli
{

class ConnectionGeometry
{
public:

    using ControlPoints = std::pair<QPointF, QPointF>;

    ConnectionGeometry();

    static ControlPoints controlPoints(QPointF start, QPointF end, ConnectionShape shape);

    /// returns the coarse bounding rect for the current connection shape.
    inline QRectF boundingRect() const { return m_rect; }
    /// returns the painter path for the current connection shape.
    inline QPainterPath const& path() const { return m_path; }
    /// returns the hitbox for the current connection shape.
    inline QPainterPath const& shape() const { return m_shape; }

    /**
     * @brief Tells the geometry to invalidate cached geometry.
     * @param start Start point of connection
     * @param end End point of connection
     * @param shape Shape of the connection
     */
    void recomputeGeometry(QPointF start, QPointF end, ConnectionShape shape);

private:

    QRectF m_rect;
    /// Path denoting connection shape
    QPainterPath m_path;
    /// Path denoting connection hitbox
    QPainterPath m_shape;

    void computePath(QPointF start, QPointF end, ConnectionShape shape);

    void computeShape(QPointF start, QPointF end, ConnectionShape shape);

    void computeBoundingRect(QPointF start, QPointF end, ConnectionShape shape);
};

} // namespace intelli

#endif // GT_INTELLI_CONNECTIONGEOMETRY_H
