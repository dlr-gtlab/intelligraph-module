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
using QtNodes::NodeId;
using QtNodes::NodeRole;
using QtNodes::ConnectionId;
using QtNodes::PortRole;


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

    m_scene = new DataFlowGraphicsScene(m_graphModel);

    connect(m_scene, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
    connect(m_scene, SIGNAL(nodeContextMenu(NodeId,QPointF)),
            SLOT(onNodeContextMenu()));

    auto view = new GraphicsView(m_scene);
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

void
NdsNodeEditor::onSelectionChanged()
{
    gtInfo() << "selected items: " << m_scene->selectedNodes().size();
}

void NdsNodeEditor::onNodeContextMenu()
{
    std::vector<NodeId> nodes = m_scene->selectedNodes();

    if (nodes.size() < 2)
    {
        return;
    }

    gtInfo() << "Multiple selection. Grouping possible!";

    QMenu menu;

    QAction* act = menu.addAction("Group Selected Nodes");

    if (menu.exec(QCursor::pos()) != act)
    {
        return;
    }

    std::unordered_set<ConnectionId> connections;
    std::unordered_set<ConnectionId> connectionsIn;
    std::unordered_set<ConnectionId> connectionsOut;

    for (NodeId nodeId: nodes)
    {
        if (m_graphModel.nodeExists(nodeId))
        {
            gtInfo() << "------------------------";
            gtInfo() << "NODE (" << nodeId << "):";
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::Type);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::Position);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::Size);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::CaptionVisible);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::Caption);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::InternalData);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::InPortCount);
            gtInfo() << m_graphModel.nodeData(nodeId, NodeRole::OutPortCount);

            // check connections

            std::unordered_set<ConnectionId> tmpConnections =
                    m_graphModel.allConnectionIds(nodeId);
            gtInfo() << "found " << tmpConnections.size() << " connections...";

            for (ConnectionId tmpConnection: tmpConnections)
            {
                if(std::find(nodes.begin(), nodes.end(), tmpConnection.inNodeId) == nodes.end())
                {
                    connectionsOut.insert(tmpConnection);
                    continue;
                }

                if(std::find(nodes.begin(), nodes.end(), tmpConnection.outNodeId) == nodes.end())
                {
                    connectionsIn.insert(tmpConnection);
                    continue;
                }

                connections.insert(tmpConnection);
            }
        }
    }

    gtInfo() << "connections to transfer: " << connections.size();
    gtInfo() << "InPorts to connect: " << connectionsIn.size();

    std::vector<NodeDataType> inDt;
    std::vector<NodeDataType> outDt;

    // eval in ports
    for (ConnectionId c: connectionsIn)
    {
        NodeId cni = c.inNodeId;
        PortIndex cpi = c.inPortIndex;
        auto pdtv = m_graphModel.portData(cni, PortType::In, cpi, PortRole::DataType);
        QtNodes::NodeDataType ndt = pdtv.value<NodeDataType>();
        gtInfo() << "in port type: " << ndt.id << "; " << ndt.name;

        inDt.push_back(ndt);
    }

    gtInfo() << "OutPorts to connect: " << connectionsOut.size();
    // eval out ports
    for (ConnectionId c: connectionsOut)
    {
        NodeId cni = c.outNodeId;
        PortIndex cpi = c.outPortIndex;

        auto pdtv = m_graphModel.portData(cni, PortType::Out, cpi, PortRole::DataType);
        QtNodes::NodeDataType ndt = pdtv.value<NodeDataType>();
        gtInfo() << "out port type: " << ndt.id << "; " << ndt.name;

        outDt.push_back(ndt);
    }

    // create group node:
    NodeId newNodeId = m_graphModel.addNode("IntelliGraph Node");

    if (!m_graphModel.nodeExists(newNodeId))
    {
        gtError() << "could not create IntelliGraph Node!";
        return;
    }

    gtInfo() << "IntelliGraph Node created (" << newNodeId << ")";

    auto delMod = m_graphModel.delegateModel<NdsExampleModel>(newNodeId);

    delMod->setInPortData(inDt);
    m_graphModel.nodeUpdated(newNodeId);
    delMod->setOutPortData(outDt);
    m_graphModel.nodeUpdated(newNodeId);

    // delete connections
    for (ConnectionId c: connectionsIn)
    {
        m_graphModel.deleteConnection(c);
    }

    for (ConnectionId c: connectionsOut)
    {
        m_graphModel.deleteConnection(c);
    }

    // make connections
    int it = 0;
    for (ConnectionId c: connectionsIn)
    {
        gtInfo() << "creating new connection...";

        ConnectionId newIn;
        newIn.inNodeId = newNodeId;
        newIn.inPortIndex = it;
        newIn.outNodeId = c.outNodeId;
        newIn.outPortIndex = c.outPortIndex;

        gtInfo() << "inNodeId = " << newIn.inNodeId << "(" << m_graphModel.nodeData(newIn.inNodeId, NodeRole::Type).toString() << ")";
        gtInfo() << "inPortIndex = " << newIn.inPortIndex;
        gtInfo() << "outNodeId = " << newIn.outNodeId << "(" << m_graphModel.nodeData(newIn.outNodeId, NodeRole::Type).toString() << ")";
        gtInfo() << "outPortIndex = " << newIn.outPortIndex;

        m_graphModel.addConnection(newIn);

        it++;
    }

    it = 0;
    for (ConnectionId c: connectionsOut)
    {
        gtInfo() << "creating new connection...";

        ConnectionId newOut;
        newOut.inNodeId = c.inNodeId;
        newOut.inPortIndex = c.inPortIndex;
        newOut.outNodeId =newNodeId;
        newOut.outPortIndex = it;

        gtInfo() << "inNodeId = " << newOut.inNodeId << "(" << m_graphModel.nodeData(newOut.inNodeId, NodeRole::Type).toString() << ")";
        gtInfo() << "inPortIndex = " << newOut.inPortIndex;
        gtInfo() << "outNodeId = " << newOut.outNodeId << "(" << m_graphModel.nodeData(newOut.outNodeId, NodeRole::Type).toString() << ")";
        gtInfo() << "outPortIndex = " << newOut.outPortIndex;

        m_graphModel.addConnection(newOut);

        it++;
    }

    // delete nodes
    for (NodeId nodeId: nodes)
    {
        m_graphModel.deleteNode(nodeId);
    }

}

