/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */


#include "gt_intelligrapheditor.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_filedialog.h"
#include "gt_application.h"
#include "gt_logging.h"
#include "gt_icons.h"
#include "gt_objectuiaction.h"
#include "gt_customactionmenu.h"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/ConnectionStyle>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QGraphicsProxyWidget>
#include <QFileInfo>
#include <QFile>

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

struct GtIntelliGraphEditor::Impl
{
    ~Impl()
    {
        if (igGraph) igGraph->clearGraphModel();
    }

    int _init_style_once = [](){
        gtApp->inDarkMode() ? setStyleDark() : setStyleBright();
        return 0;
    }();

    QMenu* sceneMenu = nullptr;

    QPointer<GtIntelliGraph> igGraph = nullptr;

    /// model
    QPointer<QtNodes::DataFlowGraphModel> model = nullptr;
    /// scene
    QtNodes::DataFlowGraphicsScene* scene = nullptr;
    /// view
    QtNodes::GraphicsView* view{new QtNodes::GraphicsView};
};

GtIntelliGraphEditor::~GtIntelliGraphEditor() = default;

GtIntelliGraphEditor::GtIntelliGraphEditor() :
    pimpl(std::make_unique<Impl>())
{
    setObjectName(tr("NodeEditor"));

    pimpl->view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(pimpl->view);
    l->setContentsMargins(0, 0, 0, 0);

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    QMenu* menu = menuBar->addMenu(tr("Scene"));
    pimpl->sceneMenu = menu;
    pimpl->sceneMenu->setEnabled(false);

    GtObjectUIAction saveAction(tr("Save"), [this](GtObject*){
        saveToJson();
    });
    saveAction.setIcon(gt::gui::icon::save());

    GtObjectUIAction loadAction(tr("Load"), [this](GtObject*){
        loadFromJson();
    });
    loadAction.setIcon(gt::gui::icon::import());

    GtObjectUIAction printGraphAction(tr("Copy to clipboard"), [this](GtObject*){
        if (!pimpl->model) return;
        QJsonDocument doc(pimpl->model->save());
        QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));
    });
    printGraphAction.setIcon(gt::gui::icon::copy());

    new GtCustomActionMenu({saveAction, loadAction, printGraphAction}, nullptr, nullptr, menu);

    /* OVERLAY */
    auto* overlay = new QVBoxLayout(pimpl->view);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

void
GtIntelliGraphEditor::setData(GtObject* obj)
{
    pimpl->igGraph = qobject_cast<GtIntelliGraph*>(obj);
    if (!pimpl->igGraph) return;

    if (pimpl->model || pimpl->scene)
    {
        gtError().verbose()
            << tr("Expected null intelli graph model and null scene!")
            << pimpl->model
            << pimpl->scene;
        return;
    }

    connect(pimpl->igGraph, &QObject::destroyed, this, &QObject::deleteLater);

    auto model = pimpl->igGraph->makeGraphModel();
    if (!model) return;

    pimpl->model = model;

    pimpl->scene = new QtNodes::DataFlowGraphicsScene(*model, model);

    connect(pimpl->scene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &GtIntelliGraphEditor::onNodeSelected);
    connect(pimpl->scene, &QtNodes::DataFlowGraphicsScene::nodeMoved,
            this, &GtIntelliGraphEditor::onNodePositionChanged);

    pimpl->view->setScene(pimpl->scene, QtNodes::GraphicsView::NoUndoRedoAction);
    pimpl->view->centerScene();

    pimpl->sceneMenu->setEnabled(true);
}

void
GtIntelliGraphEditor::loadScene(const QJsonObject& scene)
{
    if (!pimpl->model || !pimpl->scene) return;

    gtDebug().verbose()
        << "Loading JSON scene:"
        << QJsonDocument(scene).toJson(QJsonDocument::Indented);
    try
    {
        pimpl->scene->clearScene();
        pimpl->model->load(scene);
    }
    catch (std::exception const& e)
    {
        gtError() << tr("Failed to load scene from object tree! Error:")
                  << gt::quoted(std::string{e.what()});
    }
}

void
GtIntelliGraphEditor::loadFromJson()
{
    if (!pimpl->model || !pimpl->scene) return;

    QString filePath = GtFileDialog::getOpenFileName(nullptr, tr("Open Intelli Flow"));

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        gtError() << tr("Failed to open intelli flow from file! (%1)")
                     .arg(filePath);
        return;
    }

    auto scene = QJsonDocument::fromJson(file.readAll()).object();

    loadScene(scene);
}

void
GtIntelliGraphEditor::saveToJson()
{
    if (!pimpl->model) return;

    QString filePath = GtFileDialog::getSaveFileName(nullptr, tr("Save Intelli Flow"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        gtError() << tr("Failed to save IntelliFlow to file! (%1)")
                     .arg(filePath);
        return;
    }

    QJsonDocument doc(pimpl->model->save());
    file.write(doc.toJson(QJsonDocument::Indented));
}

//void
//GtIntelliGraphEditor::onNodeCreated(QtNodeId nodeId)
//{
//    if (!pimpl->igGraph || !pimpl->model || !pimpl->scene) return;

//    auto* model = pimpl->model->delegateModel<GtIntelliGraphObjectModel>(nodeId);

//    if (!model) return;

//    connect(model, &GtIntelliGraphObjectModel::nodeInitialized,
//            pimpl->scene, [=](){
//        if (auto* gobject = pimpl->scene->nodeGraphicsObject(nodeId))
//        {
//            gobject->embedQWidget();
//        }
//    });

//    pimpl->igGraph->createNode(nodeId);
//}

//void
//GtIntelliGraphEditor::onNodeDeleted(QtNodeId nodeId)
//{
//    if (!pimpl->igGraph) return;

//    pimpl->igGraph->deleteNode(nodeId);
//}

//void
//GtIntelliGraphEditor::onConnectionCreated(QtConnectionId conId)
//{
//    if (!pimpl->igGraph) return;

//    pimpl->igGraph->createConnection(conId);
//}

//void
//GtIntelliGraphEditor::onConnectionDeleted(QtConnectionId conId)
//{
//    if (!pimpl->igGraph) return;

//    pimpl->igGraph->deleteConnection(conId);
//}

void
GtIntelliGraphEditor::onNodePositionChanged(QtNodeId nodeId)
{
    if (!pimpl->igGraph) return;

    pimpl->igGraph->updateNodePosition(nodeId);
}

void
GtIntelliGraphEditor::onNodeSelected(QtNodeId nodeId)
{
    if (!pimpl->igGraph) return;

    if (auto* node = pimpl->igGraph->findNode(nodeId))
    {
        emit gtApp->objectSelected(node);
    }
}
