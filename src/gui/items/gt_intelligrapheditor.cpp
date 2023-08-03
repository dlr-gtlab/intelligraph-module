/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */


#include "gt_intelligrapheditor.h"

#include "gt_intelligraphview.h"

#include "gt_application.h"
#include "gt_logging.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QCursor>

/*
 * generated 1.2.0
 */

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
      "GradientColorVariation": 30,
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
      "GradientColorVariation": -10,
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

GtIntelliGraphEditor::GtIntelliGraphEditor() :
    m_view(new GtIntelliGraphView)
{
    gtApp->inDarkMode() ? setStyleDark() : setStyleBright();

    setObjectName(tr("IntelliGraph Editor"));

    m_view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(m_view);
    l->setContentsMargins(0, 0, 0, 0);
}

void
GtIntelliGraphEditor::setData(GtObject* obj)
{
    auto data  = qobject_cast<GtIntelliGraph*>(obj);
    if (!data)
    {
        gtError().verbose()
            << tr("Not an intelli graph!") << obj;
        return;
    }

    if (m_scene)
    {
        gtError().verbose()
            << tr("Expected null intelli graph scene, aborting!");
        return;
    }

    // close window if data was deleted
    connect(data, &QObject::destroyed, this, &QObject::deleteLater);

    // close graph model if its no longer used
    m_cleanup = gt::finally(Cleanup{data});
    m_scene = gt::ig::make_volatile<GtIntelliGraphScene>(*data);

    m_view->setScene(*m_scene);
    m_view->centerScene();

    setObjectName(tr("IntelliGraph Editor") + QStringLiteral(" - ") + data->caption());
}
