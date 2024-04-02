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

/*
static void setStyleDark()
{
    using QtNodes::GraphicsViewStyle;
    using QtNodes::NodeStyle;
    using QtNodes::ConnectionStyle;

    GraphicsViewStyle::setStyle(
        R"(
  {
    "GraphicsViewStyle": {
      "BackgroundColor": [21, 38, 53],
      "FineGridColor": [30, 47, 62],
      "CoarseGridColor": [25, 25, 25]
    }
  }
  )");

    NodeStyle::setNodeStyle(
        R"(
  {
    "NodeStyle": {
      "NormalBoundaryColor": [63, 73, 86],
      "SelectedBoundaryColor": [255, 165, 0],
      "GradientColor0": [36, 49, 63],
      "GradientColor1": [36, 49, 63],
      "GradientColor2": [36, 49, 63],
      "GradientColor3": [36, 49, 63],
      "ShadowColor": [20, 20, 20],
      "FontColor": "white",
      "FontColorFaded": "gray",
      "ConnectionPointColor": [255, 255, 255],
      "PenWidth": 1.0,
      "HoveredPenWidth": 1.5,
      "ConnectionPointDiameter": 8.0,
      "Opacity": 1.0
    }
  }
  )");

    ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "UseDataDefinedColors": true
    }
  }
  )");
}

static void setStyleBright()
{
    using QtNodes::GraphicsViewStyle;
    using QtNodes::NodeStyle;
    using QtNodes::ConnectionStyle;

    GraphicsViewStyle::setStyle(
        R"(
  {
    "GraphicsViewStyle": {
      "BackgroundColor": [255, 255, 255],
      "FineGridColor": [245, 245, 230],
      "CoarseGridColor": [235, 235, 220]
    }
  }
  )");

    NodeStyle::setNodeStyle(
        R"(
  {
    "NodeStyle": {
      "NormalBoundaryColor": "darkgray",
      "SelectedBoundaryColor": "deepskyblue",
      "GradientColor0": [245, 245, 245],
      "GradientColor1": [245, 245, 245],
      "GradientColor2": [245, 245, 245],
      "GradientColor3": [245, 245, 245],
      "ShadowColor": [200, 200, 200],
      "FontColor": [10, 10, 10],
      "FontColorFaded": [100, 100, 100],
      "ConnectionPointColor": "white",
      "PenWidth": 1.0,
      "HoveredPenWidth": 1.5,
      "ConnectionPointDiameter": 8.0,
      "Opacity": 1.0
    }
  }
  )");

    ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "UseDataDefinedColors": true
    }
  }
  )");
}
*/

struct ApplyThemeFunctor
{
    void operator()() const noexcept {
        intelli::applyTheme();
    }

    intelli::Theme lastTheme;
};

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

//    switch (theme)
//    {
//    case Theme::Bright:
//        return setStyleBright();
//    case Theme::Dark:
//        return setStyleDark();
//    case Theme::System:
//        throw std::logic_error("path is unreachable!");
//    }

//    gtError() << QObject::tr("Invalid theme!");
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
