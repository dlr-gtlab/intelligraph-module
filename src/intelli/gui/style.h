/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_STYLE_H
#define GT_INTELLI_STYLE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QColor>

namespace intelli
{

/// enum for the possible connection shapes
enum class ConnectionShape : size_t
{
    Cubic = 0,
    Straight,
    Rectangle,
    DefaultShape = Cubic
};

inline constexpr ConnectionShape
nextConnectionShape(ConnectionShape shape)
{
    switch (shape)
    {
    case ConnectionShape::Cubic:
        return ConnectionShape::Rectangle;
    case ConnectionShape::Rectangle:
        return ConnectionShape::Straight;
    case ConnectionShape::Straight:
        return ConnectionShape::Cubic;
    }
    return ConnectionShape::DefaultShape;
}

namespace style
{

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

/**
 * @brief Returns the Z-Value of the component
 * @param type Which Component
 * @return Z-Value
 */
constexpr inline double zValue(ZValue type) { return (double)type; }

/// Alias for StyleId
using StyleId = QString;

/**
 * @brief The DefaultStyle enum. Used to access predefined styles.
 */
enum DefaultStyle
{
    /// bright theme
    Bright,
    /// dark theme
    Dark
};

/**
 * @brief The StyleData class
 */
struct StyleData
{
    /// style id (will be set when registering)
    StyleId id;

    /// style data for the view
    struct ViewData
    {
        /// background color
        QColor background;
        /// color of the grid
        QColor gridline;
    } view;

    /// style data for nodes
    struct NodeData
    {
        /// background color
        QColor background;
        /// outline colors
        QColor defaultOutline, selectedOutline, hoveredOutline;
        /// outline width
        double defaultOutlineWidth = 1.0;
        double hoveredOutlineWidth = 1.5;
        double selectedOutlineWidth = 1.0;
        /// rect rounding radius
        double roundingRadius = 2.0;
        /// radius of a port
        double portRadius = 5.0;
        /// size of the eval state
        int evalStateSize = 20;
        /// size of node icon
        int iconSize = 20;
        /// modifier to apply to all r-g-b components when highlighting
        /// compatible nodes (while creatimg a draft connection)
        int compatiblityTintModifier = 20;
    } node;

    /// style data for connections
    struct ConnectionData
    {
        /// storage for custom type colors
        QHash<TypeId, QColor> customTypeColors;
        /// outline colors
        QColor defaultOutline, selectedOutline, hoveredOutline, inactiveOutline;
        /// outline width
        double defaultOutlineWidth = 3.0;
        double selectedOutlineWidth = 4.0;
        double hoveredOutlineWidth = 4.0;
        /// whether to use custom type colors
        bool useCustomTypeColors = true;
        /// whether to generate missing type colors on the fly
        bool generateMissingTypeColors  = true;

        /**
         * @brief Returns the registered or generated color of a type id.
         * @param typeId Type id
         * @return Color of the type id
         */
        GT_INTELLI_EXPORT QColor typeColor(TypeId const& typeId) const;
    } connection;
};

/**
 * @brief Applies the style with the given id
 * @param theme
 */
GT_INTELLI_EXPORT void applyStyle(StyleId const& id);
GT_INTELLI_EXPORT void applyStyle(DefaultStyle style);

/**
 * @brief Registers a new style. Styles will be registered only once
 * @param id Style id to use when switching the style
 * @param style Style data
 * @param apply Whether to apply the style as well
 */
GT_INTELLI_EXPORT void registerStyle(StyleId const& id, StyleData style, bool apply = false);

/**
 * @brief Returns the current style
 * @return Style data
 */
GT_INTELLI_EXPORT StyleData const& currentStyle();

/**
 * @brief Attempts to find the style of given style id
 * @param id Style id
 * @return Style data
 */
GT_INTELLI_EXPORT StyleData const* findStyle(StyleId const& id);
GT_INTELLI_EXPORT StyleData const* findStyle(DefaultStyle style);

/**
 * @brief Returns the id of a default style
 * @param style Default style
 * @return Style id
 */
GT_INTELLI_EXPORT StyleId const& styleId(DefaultStyle style);

/**
 * @brief Returns a list of all registered style ids
 * @return List of style ids.
 */
GT_INTELLI_EXPORT QVector<StyleId> registeredStyles();

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
 * @brief Generates a pseudo random color for a given type id.
 * The same type id yields the same type color.
 * @param typeId Type id
 * @return Color of the type id
 */
GT_INTELLI_EXPORT QColor generateTypeColor(TypeId const& typeId);
}

} // namespace intelli

#endif // GT_INTELLI_STYLE_H
