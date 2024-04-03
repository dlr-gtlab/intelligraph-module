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
intelli::style::connectionOutline(TypeId const& typeId)
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
intelli::style::connectionOutlineWidth()
{
    return 3.0;
}

QColor
intelli::style::connectionSelectedOutline()
{
    return nodeSelectedOutline();
}

QColor
intelli::style::connectionDraftOutline()
{
    return gt::gui::color::disabled();
}

double
intelli::style::connectionDraftOutlineWidth()
{
    return 3.0;
}

QColor
intelli::style::connectionHoveredOutline()
{
    return Qt::lightGray;
}

double
intelli::style::connectionHoveredOutlineWidth()
{
    return 4.0;
}

double
intelli::style::nodePortSize()
{
    return 5.0;
}

double
intelli::style::nodePortRadius()
{
    return nodePortSize() - 1.0;
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

double
intelli::style::connectionEndPointRadius()
{
    return 5.0;
}
