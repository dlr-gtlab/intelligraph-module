/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/style.h>

#include "gt_application.h"
#include "gt_logging.h"

#include <QtNodes/StyleCollection>

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

    switch (theme)
    {
    case Theme::Bright:
        return setStyleBright();
    case Theme::Dark:
        return setStyleDark();
    case Theme::System:
        throw std::logic_error("path is unreachable!");
    }

    gtError() << QObject::tr("Invalid theme!");
}

float
intelli::style::colorVariaton(ColorVariation variation)
{
    switch (variation)
    {
    case intelli::ColorVariation::Unique:
        return (gtApp->inDarkMode() ? 30 : -10);
    case intelli::ColorVariation::Graph:
        return (gtApp->inDarkMode() ? 20 : -7.5);
    case intelli::ColorVariation::Default:
    default:
        return 0.0;
    }
}


QColor
intelli::style::nodeBackground()
{
    return gtApp->inDarkMode() ? QColor{36, 49, 63} : QColor{245, 245, 245};

}

double
intelli::style::nodeOpacity()
{
    return 1;
}

QColor
intelli::style::boundarySelected()
{
    return gtApp->inDarkMode() ? QColor{255, 165, 0} : QColor{"deepskyblue"};
}

QColor
intelli::style::boundaryDefault()
{
    return gtApp->inDarkMode() ? QColor{63, 73, 86} : QColor{"darkgray"};
}

double
intelli::style::borderWidthHovered()
{
    return 1.5;
}

double
intelli::style::borderWidthDefault()
{
    return 1.0;
}
