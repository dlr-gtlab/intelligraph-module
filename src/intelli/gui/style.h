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
 */
[[deprecated]]
GT_INTELLI_EXPORT void applyTheme(Theme theme = Theme::System);

/// enum holding the zvalue of the different graphical objects
enum class ZValue : int
{
    Connection        = -2,
    ConnectionHovered = -1,
    Node              =  0,
    NodeHovered       =  1,
    NodeWidget        = 10,
    NodeEvalState     =  5,
};

namespace style
{

inline double zValue(ZValue type) { return (double)type; }

GT_INTELLI_EXPORT QColor viewBackground();

GT_INTELLI_EXPORT QColor nodeBackground();

GT_INTELLI_EXPORT QColor nodeOutline();

GT_INTELLI_EXPORT double nodeOutlineWidth();

GT_INTELLI_EXPORT QColor nodeSelectedOutline();

GT_INTELLI_EXPORT QColor nodeHoveredOutline();

GT_INTELLI_EXPORT double nodeHoveredOutlineWidth();

///
GT_INTELLI_EXPORT QColor typeIdColor(TypeId const& typeId);

GT_INTELLI_EXPORT double connectionOutlineWidth();

GT_INTELLI_EXPORT QColor connectionSelectedOutline();

GT_INTELLI_EXPORT QColor connectionDraftOutline();

GT_INTELLI_EXPORT double connectionDraftOutlineWidth();

GT_INTELLI_EXPORT QColor connectionHoveredOutline();

GT_INTELLI_EXPORT double connectionHoveredOutlineWidth();

GT_INTELLI_EXPORT double nodePortSize();

GT_INTELLI_EXPORT double nodePortRadius();

GT_INTELLI_EXPORT double nodeRoundingRadius();

GT_INTELLI_EXPORT double nodeEvalStateSize();

GT_INTELLI_EXPORT double connectionEndPointRadius();

}

} // namespace intelli

#endif // STYLE_H
