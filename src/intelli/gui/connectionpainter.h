/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_CONNECTIONPAINTER_H
#define GT_INTELLI_CONNECTIONPAINTER_H

#include <intelli/gui/style.h>

#include <QPainter>

namespace intelli
{

class ConnectionGeometry;
/**
 * @brief The ConnectionPainter class.
 * Helper class to draw connection-like objects.
 */
class ConnectionPainter
{
public:

    using PainterFlags = uint;
    /// Flags to tell the painter the state of the port. Not all flags
    enum PainterFlag : PainterFlags
    {
        NoPainterFlag    = 0,
        // Indicates that the connection should be rendered as inactive i.e. greyed out.
        // If this flag is active all other flags except `DrawDashed` and `DrawDotted`
        // are ignored depending on their priority. This flag has the 2nd highest priority.
        ObjectIsInactive = 1 << 0,
        // Indicates that the hovered outline color of the connection style should be used.
        // If this flag is active all other flags except `DrawDashed` and `DrawDotted`
        // are ignored depending on their priority. This flag has the 2nd highest priority.
        ObjectIsHovered  = 1 << 1,
        // Indicates that the selected outline color of the connection style should be used.
        // If this flag is active all other flags except `DrawDashed` and `DrawDotted`
        // are ignored depending on their priority. This flag has the 2nd highest priority.
        ObjectIsSelected = 1 << 2,
        // indicates that the connection should be drawn using dashed lines
        DrawDashed       = 1 << 3,
        // indicates that the connection should be drawn using dotted lines
        DrawDotted       = 1 << 4,
        // Indicates that the connection should be drawn with a gradient between
        // the start and end color.
        // If this flag is active all other flags except `DrawDashed` and `DrawDotted`
        // are ignored depending on their priority. This flag has the lowest priority.
        DrawGradient     = 1 << 5,
    };

    ConnectionPainter();

    /**
     * @brief Draws the connection along `path` given the connection style
     * `cstyle`. `startType` and `endType` are used to determine color of
     * connection.
     * @param painter Painter
     * @param path Path to draw
     * @param cstyle Connection style
     * @param startType TypeId of the start of the connection (Out)
     * @param endType TypeId of the end of the connection (In)
     * @param flags Flags for drawing connection
     */
    void drawPath(QPainter& painter,
                  QPainterPath const& path,
                  style::StyleData::ConnectionData const& cstyle,
                  TypeId const& startType,
                  TypeId const& endType,
                  PainterFlags flags = NoPainterFlag) const;

    /**
     * @brief Draws the connection along `path` given the connection style
     * `cstyle`. `startColor` and `endColor` are used to draw the connection
     * respectively. If the `DrawGradient` flag is active, a linear gradient
     * is drawn between the start and end position of the path.
     * @param painter Painter
     * @param path Path to draw
     * @param cstyle Connection style
     * @param startColor Color for the start of the path. This color is
     * used exclusively if the `DrawGradient` flag is not active.
     * @param endColor Color for the end of the path. This color is
     * only used in conjuction with the `DrawGradient` flag.
     * @param flags Flags for drawing connection
     */
    void drawPath(QPainter& painter,
                  QPainterPath const& path,
                  style::StyleData::ConnectionData const& cstyle,
                  QColor const& startColor,
                  QColor const& endColor,
                  PainterFlags flags = NoPainterFlag) const;

    /**
     * @brief Draws the connection along `path` given the connection style
     * `cstyle`. The default outline color of `cstyle` is used for drawing
     * the connection.
     * @param painter Painter
     * @param path Path to draw
     * @param cstyle Connection style
     * @param flags Flags for drawing connection
     */
    void drawPath(QPainter& painter,
                  QPainterPath const& path,
                  style::StyleData::ConnectionData const& cstyle,
                  PainterFlags flags = NoPainterFlag) const;

    /**
     * @brief Draws an end point for the connection. Note: uses cached painter
     * settings for pen and brush.
     * @param painter Painter
     * @param path Path of the connection
     * @param radius Radius of the end point
     * @param type On which end the end point should be displayed
     */
    void drawEndPoint(QPainter& painter,
                      QPainterPath const& path,
                      double radius,
                      PortType type = PortType::In) const;

private:

    /// helper method for setting up the painter
    void applyPenConfig(QPainter& painter,
                        style::StyleData::ConnectionData const& cstyle,
                        QColor const& startColor,
                        QColor const& endColor,
                        QPointF start,
                        QPointF end,
                        PainterFlags flags = NoPainterFlag) const;
};

} // namespace intelli

#endif // GT_INTELLI_CONNECTIONPAINTER_H
