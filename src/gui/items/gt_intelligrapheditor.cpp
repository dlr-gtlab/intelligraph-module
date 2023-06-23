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

#include "gt_intelligraphnodegroup.h"

#include "models/gt_intelligraphobjectmodel.h"

#include "gt_filedialog.h"
#include "gt_application.h"
#include "gt_logging.h"
#include "gt_icons.h"

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

#define GT_REGISTER_NODE_MODEL(REGISTRY, T, CAT) \
{ \
    REGISTRY->registerModel<GtIntelliGraphObjectModel>( \
                [](){ \
        return std::make_unique<GtIntelliGraphObjectModel>(GT_CLASSNAME(T)); \
    }, CAT); \
}

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

struct GtIntelliGraphEditor::Impl
{
    int _init_style_once = [](){
        gtApp->inDarkMode() ? setStyleDark() : setStyleBright();
        return 0;
    }();

    QPointer<GtIntelliGraph> igGraph = nullptr;

    /// model
    QtNodes::DataFlowGraphModel model{
        GtIntelliGraphNodeFactory::instance().makeRegistry()
    };
    /// scene
    QtNodes::DataFlowGraphicsScene* scene{new QtNodes::DataFlowGraphicsScene(model, &model)};
    /// view
    QtNodes::GraphicsView* view{new QtNodes::GraphicsView(scene)};
};

GtIntelliGraphEditor::GtIntelliGraphEditor() :
    pimpl(std::make_unique<Impl>())
{
    setObjectName(tr("NodeEditor"));

    pimpl->view->setFrameShape(QFrame::NoFrame);

    // hacky way to disable undo/redo actions
    // TODO: Add switch to QtNodes lib for disabling undo/redo actions
    for (auto* action : pimpl->view->actions())
    {
        auto shortcut = action->shortcut();
        if (shortcut == QKeySequence::Undo || shortcut == QKeySequence::Redo)
        {
            action->setEnabled(false);
            action->setShortcut(QKeySequence{});
        }
    }

    QVBoxLayout* l = new QVBoxLayout(widget());
    l->addWidget(pimpl->view);
    l->setContentsMargins(0, 0, 0, 0);

    connect(&pimpl->model, &QtNodes::DataFlowGraphModel::sceneLoaded,
            pimpl->view, &QtNodes::GraphicsView::centerScene);

    connect(pimpl->scene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &GtIntelliGraphEditor::onNodeSelected);

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    QMenu *menu = menuBar->addMenu(tr("Scene"));

    auto saveAction = menu->addAction(tr("Save..."));
    saveAction->setIcon(gt::gui::icon::save());

    auto loadAction = menu->addAction(tr("Load..."));
    loadAction->setIcon(gt::gui::icon::import());

    auto printGraphAction = menu->addAction(tr("Copy to clipboard"));
    printGraphAction->setIcon(gt::gui::icon::copy());

    menu->addSeparator();

    QObject::connect(saveAction, &QAction::triggered,
                     this, &GtIntelliGraphEditor::saveToJson);
    QObject::connect(loadAction, &QAction::triggered,
                     this, &GtIntelliGraphEditor::loadFromJson);
    QObject::connect(printGraphAction, &QAction::triggered,
                     &pimpl->model, [&](){
        QJsonDocument doc(pimpl->model.save());
        QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));
    });

    /* OVERLAY */
    auto* overlay = new QVBoxLayout(pimpl->view);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

GtIntelliGraphEditor::~GtIntelliGraphEditor()
{
    gtTrace() << __FUNCTION__;
}

void
GtIntelliGraphEditor::loadScene(const QJsonObject& scene)
{
    gtDebug().verbose()
            << "Loading JSON scene:"
            << QJsonDocument(scene).toJson(QJsonDocument::Indented);
    try
    {
        pimpl->scene->clearScene();
        pimpl->model.load(scene);
    }
    catch (std::exception const& e)
    {
        gtError() << tr("Failed to load scene from object tree! Error:")
                  << gt::quoted(std::string{e.what()});
    }
}

void
GtIntelliGraphEditor::setData(GtObject* obj)
{
    pimpl->igGraph = qobject_cast<GtIntelliGraph*>(obj);

    if (pimpl->igGraph)
    {
        pimpl->igGraph->setActiveGraphModel(pimpl->model);

        connect(pimpl->igGraph, &QObject::destroyed,
                this, &QObject::deleteLater);

        connect(&pimpl->model, &QtNodes::DataFlowGraphModel::nodeCreated,
                this, &GtIntelliGraphEditor::onNodeCreated);
        connect(&pimpl->model, &QtNodes::DataFlowGraphModel::nodeDeleted,
                this, &GtIntelliGraphEditor::onNodeDeleted);
        connect(pimpl->scene, &QtNodes::DataFlowGraphicsScene::nodeMoved,
                this, &GtIntelliGraphEditor::onNodePositionChanged);
        connect(&pimpl->model, &QtNodes::DataFlowGraphModel::connectionCreated,
                this, &GtIntelliGraphEditor::onConnectionCreated);
        connect(&pimpl->model, &QtNodes::DataFlowGraphModel::connectionDeleted,
                this, &GtIntelliGraphEditor::onConnectionDeleted);

        // once loaded remove all orphan nodes and connections
        connect(&pimpl->model, &QtNodes::DataFlowGraphModel::sceneLoaded,
                pimpl->igGraph, [=](){
            pimpl->igGraph->removeOrphans(pimpl->model);
        });

        auto scene = pimpl->igGraph->toJson();

        loadScene(scene);
    }
}

void
GtIntelliGraphEditor::loadFromJson()
{
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
    QString filePath = GtFileDialog::getSaveFileName(nullptr, tr("Save Intelli Flow"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        gtError() << tr("Failed to save IntelliFlow to file! (%1)")
                     .arg(filePath);
        return;
    }

    QJsonDocument doc(pimpl->model.save());
    file.write(doc.toJson(QJsonDocument::Indented));
}

void
GtIntelliGraphEditor::onNodeCreated(QtNodeId nodeId)
{
    if (!pimpl->igGraph) return;

    auto* model = pimpl->model.delegateModel<GtIntelliGraphObjectModel>(nodeId);

    if (!model) return;

    connect(model, &GtIntelliGraphObjectModel::nodeInitialized,
            pimpl->scene, [=](){
        if (auto* gobject = pimpl->scene->nodeGraphicsObject(nodeId))
        {
            gobject->embedQWidget();
        }
    });

    pimpl->igGraph->createNode(nodeId);
}

void
GtIntelliGraphEditor::onNodeDeleted(QtNodeId nodeId)
{
    if (!pimpl->igGraph) return;

    pimpl->igGraph->deleteNode(nodeId);
}

void
GtIntelliGraphEditor::onConnectionCreated(QtConnectionId conId)
{
    if (!pimpl->igGraph) return;

    pimpl->igGraph->createConnection(conId);
}

void
GtIntelliGraphEditor::onConnectionDeleted(QtConnectionId conId)
{
    if (!pimpl->igGraph) return;

    pimpl->igGraph->deleteConnection(conId);
}

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
