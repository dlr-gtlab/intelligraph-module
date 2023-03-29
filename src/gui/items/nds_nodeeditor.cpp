/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#include "gt_application.h"

#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModelRegistry>

#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>
#include <QAction>

#include <QtGui/QScreen>

/*
 * generated 1.2.0
 */
 
#include "nds_nodeeditor.h"
#include "nds_objectloadermodel.h"
#include "nds_objectmementomodel.h"
#include "nds_shapegenmodel.h"
#include "nds_shapevisualizationmodel.h"
#include "nds_wireframemodel.h"
#include "nds_combineshapesmodel.h"
#include "nds_shapesettingsmodel.h"
#include "ndsshapecolormodel.h"
//#include "nds_qmlpiechartmodel.h"
//#include "nds_qmlbarchartmodel.h"
//#include "ndsqmllinechartmodel.h"
#include "nds_qwtbarchartmodel.h"
#include "nds_examplemodel.h"

using QtNodes::BasicGraphicsScene;
using QtNodes::ConnectionStyle;
using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::GraphicsViewStyle;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeStyle;


static void setStyle()
{
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
      "GradientColor0": [255, 255, 255],
      "GradientColor1": [255, 255, 255],
      "GradientColor2": [255, 255, 255],
      "GradientColor3": [255, 255, 255],
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

static std::shared_ptr<NodeDelegateModelRegistry> registerDataModels()
{
    auto ret = std::make_shared<NodeDelegateModelRegistry>();
    ret->registerModel<NdsObjectLoaderModel>("Object");
    ret->registerModel<NdsObjectMementoModel>("Object");
    ret->registerModel<NdsShapeGenModel>("3D Shapes");
    ret->registerModel<NdsShapeVisualizationModel>("3D Shapes");
    ret->registerModel<NdsWireframeModel>("3D Shapes");
    ret->registerModel<NdsCombineShapesModel>("3D Shapes");
    ret->registerModel<NdsShapeSettingsModel>("3D Shapes");
    ret->registerModel<NdsShapeColorModel>("3D Shapes");
//    ret->registerModel<NdsQmlPieChartModel>("Qml");
//    ret->registerModel<NdsQmlBarChartModel>("Qml");
//    ret->registerModel<NdsQmlLineChartModel>("Qml");
    ret->registerModel<NdsQwtBarChartModel>("Qwt");
    ret->registerModel<NdsExampleModel>("Misc");

    return ret;
}

NdsNodeEditor::NdsNodeEditor() :
    m_graphModel(registerDataModels())
{
    setObjectName("NodeEditor");

    if (gtApp->inDarkMode())
    {
        setStyle();
    }
    else
    {
        setStyleBright();
    }

    QVBoxLayout* l = new QVBoxLayout(widget());

    auto scene = new DataFlowGraphicsScene(m_graphModel);

    auto view = new GraphicsView(scene);
    l->addWidget(view);
    l->setContentsMargins(0, 0, 0, 0);
}

void
NdsNodeEditor::setData(GtObject *obj)
{
//    gtInfo() << "SET DATA NODES";
}

void
NdsNodeEditor::showEvent()
{
//    gtInfo() << "SHOW EVENT NODES";

//    // Initialize and connect two nodes.
//    {
//        NodeId id1 = m_graphModel.addNode();
//        m_graphModel.setNodeData(id1, NodeRole::Position, QPointF(0, 0));

//        NodeId id2 = m_graphModel.addNode();
//        m_graphModel.setNodeData(id2, NodeRole::Position, QPointF(300, 300));

//        m_graphModel.addConnection(ConnectionId{id1, 0, id2, 0});
//    }
}

