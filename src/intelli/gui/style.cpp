/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/style.h>

#include "gt_application.h"
#include "gt_colors.h"
#include "gt_utilities.h"

#include <random>

void
intelli::applyTheme(Theme newTheme)
{
    static Theme theme = newTheme;

    static auto connect_once = [](){
        QObject::connect(gtApp, &GtApplication::themeChanged, gtApp, [](){
            if (theme == Theme::System) applyTheme(theme);
        });
        return 0;
    }();
    Q_UNUSED(connect_once);

    // apply system theme
    if (theme == Theme::System)
    {
        theme = gtApp->inDarkMode() ? Theme::Dark : Theme::Bright;
    }
}

QColor
intelli::style::tint(QColor const& color, int r, int g, int b)
{
    constexpr int MAX = std::numeric_limits<uint8_t>::max();
    constexpr int MIN = std::numeric_limits<uint8_t>::min();

    return QColor{
        gt::clamp(color.red()   + r, MIN, MAX),
        gt::clamp(color.green() + g, MIN, MAX),
        gt::clamp(color.blue()  + b, MIN, MAX)
    };
}

QColor
intelli::style::invert(QColor const& color)
{
    constexpr int MAX = std::numeric_limits<uint8_t>::max();

    return QColor{MAX - color.red(), MAX - color.green(), MAX - color.blue()};
}


QColor
intelli::style::viewBackground()
{
    return gtApp->inDarkMode() ? QColor{21, 38, 53} : QColor{255, 255, 255};
}

QColor
intelli::style::nodeBackground()
{
    return gtApp->inDarkMode() ? QColor{36, 49, 63} : QColor{245, 245, 245};
}

QColor
intelli::style::nodeOutline()
{
    return gtApp->inDarkMode() ? QColor{63, 73, 86} : QColor{"darkgray"};
}

double
intelli::style::nodeOutlineWidth()
{
    return 1.0;
}

QColor
intelli::style::nodeSelectedOutline()
{
    return gtApp->inDarkMode() ? QColor{255, 165, 0} : QColor{"deepskyblue"};
}

double
intelli::style::nodeSelectedOutlineWidth()
{
    return nodeOutlineWidth();
}

QColor
intelli::style::nodeHoveredOutline()
{
    return nodeOutline();
}

double
intelli::style::nodeHoveredOutlineWidth()
{
    return 1.5;
}

QColor
intelli::style::typeIdColor(TypeId const& typeId)
{
    static QHash<QString, QColor> cache;

    if (typeId.isEmpty()) return Qt::darkCyan;

    auto iter = cache.find(typeId);
    if (iter != cache.end())
    {
        return *iter;
    }

    // from QtNodes
    std::size_t hash = qHash(typeId);

    std::size_t const hue_range = 0xFF;

    std::mt19937 gen(static_cast<unsigned int>(hash));
    std::uniform_int_distribution<int> distrib(0, hue_range);

    int hue = distrib(gen);
    int sat = 120 + hash % 129;

    QColor color = QColor::fromHsl(hue, sat, 160);
    cache.insert(typeId, color);
    return color;
}

double
intelli::style::connectionPathWidth()
{
    return 3.0;
}

QColor
intelli::style::connectionSelectedPath()
{
    return nodeSelectedOutline();
}

double
intelli::style::connectionSelectedPathWidth()
{
    return 2 * connectionPathWidth();
}

QColor
intelli::style::connectionDraftPath()
{
    return gt::gui::color::disabled();
}

double
intelli::style::connectionDraftPathWidth()
{
    return connectionPathWidth();
}

QColor
intelli::style::connectionHoveredPath()
{
    return Qt::lightGray;
}

double
intelli::style::connectionHoveredPathWidth()
{
    return 1 + connectionPathWidth();
}

double
intelli::style::nodePortSize()
{
    return 5.0;
}

double
intelli::style::nodeRoundingRadius()
{
    return 2.0;
}

double
intelli::style::nodeEvalStateSize()
{
    return 20.0;
}
