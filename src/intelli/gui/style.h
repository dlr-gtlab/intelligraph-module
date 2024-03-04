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

namespace intelli
{

/**
 * @brief The Theme enum
 */
enum class Theme
{
    /// bright mode
    Bright,
    /// dark mode
    Dark,
    /// automatic theme selection
    System
};

enum class ColorVariation
{
    Default,
    Unique,
    Graph,

};

/**
 * @brief Applies the node style base on the current theme. System will adapt
 * the bright or dark mode. The system theme will be upated automatically.
 * @param theme
 */
GT_INTELLI_EXPORT void applyTheme(Theme theme = Theme::System);


namespace style
{

GT_INTELLI_EXPORT float colorVariaton(ColorVariation variation);

}

} // namespace intelli

#endif // STYLE_H
