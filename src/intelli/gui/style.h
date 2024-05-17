/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef STYLE_H
#define STYLE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QColor>

namespace intelli
{

/**
 * @brief The Theme enum
 * @deprecated No longer used
 */
enum class [[deprecated]] Theme
{
    /// bright mode
    Bright,
    /// dark mode
    Dark,
    /// automatic theme selection
    System
};

/**
 * @brief Applies the node style base on the current theme. System will adapt
 * the bright or dark mode. The system theme will be upated automatically.
 * @param theme
 * @deprecated No longer used since it has not affect anymore
 */
[[deprecated]]
GT_INTELLI_EXPORT void applyTheme(Theme theme = Theme::System);

/// enum holding the zvalue of the different graphical objects
enum class ZValue : int
{
    Connection        = -2,
    ConnectionHovered = -1,
    DraftConnection   =  2,
    Node              =  0,
    NodeHovered       =  1,
    NodeWidget        = 10,
    NodeEvalState     =  5,
};

namespace style
{

/**
 * @brief Returns the Z-Value of the component
 * @param type Which Component
 * @return Z-Value
 */
constexpr inline double zValue(ZValue type) { return (double)type; }

/**
 * @brief Applies a tint to `color` (additively, regardless of theme).
 * @param color Color to tint
 * @param r Red value
 * @param g Green value
 * @param b Blue value
 * @return Tinted color
 */
GT_INTELLI_EXPORT QColor tint(QColor const& color, int r, int g, int b);

/**
 * @brief Overload, applies value to red, green, andblue.
 * @param color Color to tint
 * @param val Value to apply to red, green, and blue.
 * @return Tinted color
 */
inline QColor tint(QColor const& color, int val) { return tint(color, val, val, val); }

/**
 * @brief Inverts `color`, regardless of theme.
 * @param color Color to invert
 * @return Inverted color
 */
GT_INTELLI_EXPORT QColor invert(QColor const& color);

/**
 * @brief Background color of the graph view
 * @return Color
 */
GT_INTELLI_EXPORT QColor viewBackground();

/**
 * @brief Default background color of nodes
 * @return Color
 */
GT_INTELLI_EXPORT QColor nodeBackground();

/**
 * @brief Default outline color of nodes (not selected or hovered)
 * @return Color
 */
GT_INTELLI_EXPORT QColor nodeOutline();

/**
 * @brief Default outline width of nodes
 * @return Width
 */
GT_INTELLI_EXPORT double nodeOutlineWidth();

/**
 * @brief Outline color of selected nodes
 * @return Color
 */
GT_INTELLI_EXPORT QColor nodeSelectedOutline();

/**
 * @brief Outline width of selected nodes
 * @return Width
 */
GT_INTELLI_EXPORT double nodeSelectedOutlineWidth();

/**
 * @brief Outline color of hovered nodes
 * @return Color
 */
GT_INTELLI_EXPORT QColor nodeHoveredOutline();

/**
 * @brief Outline width of hovered nodes
 * @return Width
 */
GT_INTELLI_EXPORT double nodeHoveredOutlineWidth();

/**
 * @brief Returns the color associated to a typeId. Used for connections and
 * ports.
 * @param typeId TypeId to get color from
 * @return TypeId color
 */
GT_INTELLI_EXPORT QColor typeIdColor(TypeId const& typeId);

/**
 * @brief Default outline width of connections
 * @return Width
 */
GT_INTELLI_EXPORT double connectionPathWidth();

/**
 * @brief Color of selected connections
 * @return Color
 */
GT_INTELLI_EXPORT QColor connectionSelectedPath();

/**
 * @brief Width of selected connections
 * @return Width
 */
GT_INTELLI_EXPORT double connectionSelectedPathWidth();

/**
 * @brief Color of draft connections
 * @return Color
 */
GT_INTELLI_EXPORT QColor connectionDraftPath();

/**
 * @brief Color of inactive connections
 * @return Color
 */
GT_INTELLI_EXPORT QColor connectionInactivePath();

/**
 * @brief Width of draft connections
 * @return Width
 */
GT_INTELLI_EXPORT double connectionDraftPathWidth();

/**
 * @brief Color of hovered connections
 * @return Color
 */
GT_INTELLI_EXPORT QColor connectionHoveredPath();

/**
 * @brief Width of hovered connections
 * @return Width
 */
GT_INTELLI_EXPORT double connectionHoveredPathWidth();

/**
 * @brief Size (height and width) of ports.
 * @return Width and Height
 */
GT_INTELLI_EXPORT double nodePortSize();

/**
 * @brief Radius of nodes rect.
 * @return Radius
 */
GT_INTELLI_EXPORT double nodeRoundingRadius();

/**
 * @brief Size (height and width) of the eval state rect.
 * @return Width and Height
 */
GT_INTELLI_EXPORT double nodeEvalStateSize();

}

} // namespace intelli

#endif // STYLE_H
