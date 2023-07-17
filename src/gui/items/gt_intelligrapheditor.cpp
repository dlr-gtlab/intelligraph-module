/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */


#include "gt_intelligrapheditor.h"

#include "gt_intelligraphconnection.h"
#include "gt_intelligraphdatafactory.h"
#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"

#include "gt_filedialog.h"
#include "gt_application.h"
#include "gt_logging.h"
#include "gt_icons.h"
#include "gt_objectuiaction.h"
#include "gt_customactionmenu.h"
#include "gt_objectmemento.h"
#include "gt_qtutilities.h"
#include "gt_guiutilities.h"
#include "gt_inputdialog.h"
#include "gt_objectfactory.h"
#include "gt_command.h"
#include "gt_datamodel.h"

#include <QtNodes/StyleCollection>
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

GtIntelliGraphEditor::~GtIntelliGraphEditor()
{
    if (m_data) m_data->clearGraphModel();
}

GtIntelliGraphEditor::GtIntelliGraphEditor() :
    m_view(new QtNodes::GraphicsView)
{
    gtApp->inDarkMode() ? setStyleDark() : setStyleBright();

    setObjectName(tr("NodeEditor"));

    m_view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(m_view);
    l->setContentsMargins(0, 0, 0, 0);

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    QMenu* menu = menuBar->addMenu(tr("Scene"));
    m_sceneMenu = menu;
    m_sceneMenu->setEnabled(false);

    GtObjectUIAction saveAction(tr("Save"), [this](GtObject*){
        saveToJson();
    });
    saveAction.setIcon(gt::gui::icon::save());

    GtObjectUIAction loadAction(tr("Load"), [this](GtObject*){
        loadFromJson();
    });
    loadAction.setIcon(gt::gui::icon::import());

    GtObjectUIAction printGraphAction(tr("Copy to clipboard"), [this](GtObject*){
        if (!m_model) return;
        QJsonDocument doc(m_model->save());
        QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));
    });
    printGraphAction.setIcon(gt::gui::icon::copy());

    new GtCustomActionMenu({saveAction, loadAction, printGraphAction}, nullptr, nullptr, menu);

    /* OVERLAY */
    auto* overlay = new QVBoxLayout(m_view);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

void
GtIntelliGraphEditor::setData(GtObject* obj)
{
    m_data = qobject_cast<GtIntelliGraph*>(obj);
    if (!m_data) return;

    if (m_model || m_scene)
    {
        gtError().verbose()
            << tr("Expected null intelli graph model and null scene!")
            << m_model
            << m_scene;
        return;
    }

    connect(m_data, &QObject::destroyed, this, &QObject::deleteLater);

    auto model = m_data->makeGraphModel();
    if (!model) return;

    m_model = model;

    m_scene = new QtNodes::DataFlowGraphicsScene(*model, model);

    connect(m_scene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &GtIntelliGraphEditor::onNodeSelected);
    connect(m_scene, &QtNodes::DataFlowGraphicsScene::nodeDoubleClicked,
            this, &GtIntelliGraphEditor::onNodeDoubleClicked);
    connect(m_scene, &QtNodes::DataFlowGraphicsScene::nodeMoved,
            this, &GtIntelliGraphEditor::onNodePositionChanged);
    connect(m_scene, &QtNodes::DataFlowGraphicsScene::nodeContextMenu,
            this, &GtIntelliGraphEditor::onNodeContextMenu);
    connect(m_scene, &QtNodes::DataFlowGraphicsScene::portContextMenu,
            this, &GtIntelliGraphEditor::onPortContextMenu);


    m_view->setScene(m_scene, QtNodes::GraphicsView::NoUndoRedoAction);
    m_view->centerScene();

    m_sceneMenu->setEnabled(true);
}

void
GtIntelliGraphEditor::loadScene(const QJsonObject& scene)
{
    assert(m_model); assert(m_scene);

    gtDebug().verbose()
        << "Loading JSON scene:"
        << QJsonDocument(scene).toJson(QJsonDocument::Indented);
    try
    {
        m_scene->clearScene();
        m_model->load(scene);
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
    assert(m_model); assert(m_scene);

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
    assert(m_model);

    QString filePath = GtFileDialog::getSaveFileName(nullptr, tr("Save Intelli Flow"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        gtError() << tr("Failed to save IntelliFlow to file! (%1)")
                     .arg(filePath);
        return;
    }

    QJsonDocument doc(m_model->save());
    file.write(doc.toJson(QJsonDocument::Indented));
}

void
GtIntelliGraphEditor::onNodePositionChanged(QtNodeId nodeId)
{
    assert(m_data);

    m_data->updateNodePosition(nodeId);
}

void
GtIntelliGraphEditor::onNodeSelected(QtNodeId nodeId)
{
    assert(m_data);

    if (auto* node = m_data->findNode(nodeId))
    {
        emit gtApp->objectSelected(node);
    }
}

void
GtIntelliGraphEditor::onNodeDoubleClicked(QtNodeId nodeId)
{
    assert(m_data);

    auto* node = m_data->findNode(nodeId);
    if (!node) return;

    gt::gui::handleObjectDoubleClick(*node);
}

void
GtIntelliGraphEditor::onPortContextMenu(QtNodeId nodeId, QtPortType type, QtPortIndex idx, QPointF pos)
{
    assert(m_data); assert(m_model);

    // create menu
    QMenu menu;

    auto conIds = m_model->allConnectionIds(nodeId);

    QList<GtObject*> connections;
    connections.reserve(conIds.size());

    for (auto conId : conIds)
    {
        if (auto* con = m_data->findConnection(conId))
        {
            connections.append(con);
        }
    }

    QAction* deleteAct = menu.addAction(tr("Remove connections"));
    deleteAct->setEnabled(!connections.empty());
    deleteAct->setIcon(gt::gui::icon::chainOff());

    QAction* res = menu.exec(QCursor::pos());

    if (res == deleteAct)
    {
        gtDataModel->deleteFromModel(connections);
    }
}

void
GtIntelliGraphEditor::onNodeContextMenu(QtNodeId nodeId, QPointF pos)
{
    assert(m_data); assert(m_model); assert(m_scene);

    // retrieve selected nodes
    auto selectedNodeIds = m_scene->selectedNodes();

    bool allDeletable = std::all_of(selectedNodeIds.begin(),
                                    selectedNodeIds.end(),
                                    [this](QtNodeId id){
        return m_model->nodeFlags(id) & QtNodes::Deletable;
    });

    // create menu
    QMenu menu;

    QAction* groupAct = menu.addAction(tr("Group selected Nodes"));
    groupAct->setIcon(gt::gui::icon::select());
    groupAct->setEnabled(allDeletable);
    groupAct->setVisible(!selectedNodeIds.empty());

    menu.addSeparator();

    QAction* deleteAct = menu.addAction(tr("Delete selected Nodes"));
    deleteAct->setIcon(gt::gui::icon::delete_());
    deleteAct->setEnabled(allDeletable);

    // add node to selected nodes
    if (selectedNodeIds.empty())
    {
        selectedNodeIds.push_back(nodeId);
    }

    // add custom object menu
    if (selectedNodeIds.size() == 1)
    {
        if (GtIntelliGraphNode* node = m_data->findNode(selectedNodeIds.front()))
        {
            menu.addSeparator();

            gt::gui::makeObjectContextMenu(menu, *node);
        }
        deleteAct->setVisible(false);
    }

    QAction* res = menu.exec(QCursor::pos());

    if (res == groupAct)
    {
        return makeGroupNode(selectedNodeIds);
    }
    if (res == deleteAct)
    {
        return deleteNodes(selectedNodeIds);
    }
}

void
GtIntelliGraphEditor::deleteNodes(const std::vector<QtNodeId>& nodeIds)
{
    QList<GtObject*> nodes;
    nodes.reserve(nodeIds.size());

    for (QtNodeId nodeId : nodeIds)
    {
        if (auto node = m_data->findNode(nodeId))
        {
            nodes.push_back(node);
        }
    }

    gtDataModel->deleteFromModel(nodes);
}

void
GtIntelliGraphEditor::makeGroupNode(std::vector<QtNodeId> const& selectedNodeIds)
{
    using PortIndex      = gt::ig::PortIndex;
    using NodeId         = gt::ig::NodeId;
    using QtNodeRole     = QtNodes::NodeRole;
    using QtNodeDataType = QtNodes::NodeDataType;
    using QtPortRole     = QtNodes::PortRole;
    using QtConnectionId = QtNodes::ConnectionId;

    // get new node name
    GtInputDialog dialog(GtInputDialog::TextInput);
    dialog.setWindowTitle(tr("New Node Caption"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(tr("Enter a new caption for the grouped nodes"));
    dialog.setInitialTextValue(QStringLiteral("Sub Graph"));
    if (!dialog.exec()) return;

    QString const& groupNodeName = dialog.textValue();

    // find input/output connections and connections to move
    std::vector<QtConnectionId> connectionsInternal, connectionsIn, connectionsOut;

    for (QtNodeId nodeId : selectedNodeIds)
    {
        if (!m_model->nodeExists(nodeId)) continue;

        // check connections
        for (QtConnectionId conId : m_model->allConnectionIds(nodeId))
        {
            if(std::find(selectedNodeIds.begin(), selectedNodeIds.end(),
                          conId.inNodeId) == selectedNodeIds.end())
            {
                connectionsOut.push_back(conId);
                continue;
            }

            if(std::find(selectedNodeIds.begin(), selectedNodeIds.end(),
                          conId.outNodeId) == selectedNodeIds.end())
            {
                connectionsIn.push_back(conId);
                continue;
            }

            connectionsInternal.push_back(conId);
        }
    }

    // find datatype for input provider
    std::vector<QString> dtypeIn;
    for (QtConnectionId c : connectionsIn)
    {
        QtNodeId nodeId = c.inNodeId;
        QtPortIndex port = c.inPortIndex;

        auto dtype = m_model->portData(nodeId, QtPortType::In, port, QtPortRole::DataType)
                         .value<QtNodeDataType>();

        if (!GtIntelliGraphDataFactory::instance().knownClass(dtype.id))
        {
            auto const& name = m_model->nodeData(nodeId, QtNodeRole::Caption);
            gtError() << tr("Failed to create group node! "
                            "(Unkown node datatype '%1', id: %2, port: %3)")
                            .arg(dtype.id, name.toString()).arg(port);
        }

        dtypeIn.push_back(dtype.id);
    }

    // find datatypes for output provider
    std::vector<QString> dtypeOut;
    for (QtConnectionId c : connectionsOut)
    {
        QtNodeId nodeId = c.outNodeId;
        QtPortIndex port = c.outPortIndex;

        auto dtype = m_model->portData(nodeId, QtPortType::Out, port, QtPortRole::DataType)
                         .value<QtNodeDataType>();

        if (!GtIntelliGraphDataFactory::instance().knownClass(dtype.id))
        {
            auto const& name = m_model->nodeData(nodeId, QtNodeRole::Caption);
            gtError() << tr("Failed to create group node! "
                            "(Unkown node datatype '%1', id: %2, port: %3)")
                             .arg(dtype.id, name.toString()).arg(port);
        }

        dtypeOut.push_back(dtype.id);
    }

    // create group node
    QtNodeId groupNodeId = m_model->addNode(m_data->modelName());
    auto* groupNode = qobject_cast<GtIntelliGraph*>(m_data->findNode(groupNodeId));
    if (!groupNode || !groupNode->activeGraphModel())
    {
        gtError() << tr("Failed to create group node! (Invalid group node)");
        return;
    }

    auto groupModel = groupNode->activeGraphModel();
    groupNode->setCaption(groupNodeName);

    // setup input/output provider
    auto* inputProvider = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << tr("Failed to create group node! "
                        "(Invalid input or output provider)");
        m_data->deleteNode(groupNodeId);
        return;
    }

    for (QString const& typeId : dtypeIn)
    {
        inputProvider->insertPort(typeId);
    }

    for (QString const& typeId : dtypeOut)
    {
        outputProvider->insertPort(typeId);
    }

    // preprocess selected nodes
    QPolygonF selectionPoly;

    QVector<GtIntelliGraphNode*> selectedNodes;

    for (QtNodeId nodeId : selectedNodeIds)
    {
        auto node = m_data->findNode(nodeId);
        if (!node)
        {
            gtError() << tr("Failed to create group node! "
                            "(Node %1 not found)").arg(nodeId);
            return;
        }
        selectedNodes.append(node);
        selectionPoly.append(node->pos());
    }

    // update node positions
    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    auto offset = QPointF{boundingRect.width() / 2, boundingRect.height() / 2};

    m_data->setNodePosition(groupNodeId, center);
    groupNode->setNodePosition(inputProvider->id(), inputProvider->pos() - offset);
    groupNode->setNodePosition(outputProvider->id(), outputProvider->pos() + offset);

    // move selected nodes
    for (auto* node : selectedNodes)
    {
        auto newNode = gt::unique_qobject_cast<GtIntelliGraphNode>(
            node->toMemento().toObject(*gtObjectFactory)
        );

        if (!newNode)
        {
            gtError() << tr("Failed to create group node! "
                            "(Nodes %1 could not be copied)").arg(node->id());
            return;
        }

        newNode->setPos(newNode->pos() - center);

        // append new node
        auto* movedNode = groupNode->appendNode(std::move(newNode));
        if (!movedNode)
        {
            gtError() << tr("Failed to create group node! "
                            "(Node could not be moved)").arg(node->id());
            return;
        }

        NodeId oldId = node->id();
        NodeId newId = movedNode->id();

        m_data->deleteNode(oldId);

        // udpate connections if node id has changed
        if (newId == oldId) continue;

        gtDebug().verbose() << "Updating node id from" << oldId << "to" << newId << "...";

        for (auto& conId : connectionsIn)
        {
            if (conId.inNodeId == oldId) conId.inNodeId = newId;
        }
        for (auto& conId : connectionsOut)
        {
            if (conId.outNodeId == oldId) conId.outNodeId = newId;
        }
        for (auto& conId : connectionsInternal)
        {
            if (conId.inNodeId == oldId) conId.inNodeId = newId;
            if (conId.outNodeId == oldId) conId.outNodeId = newId;
        }
    }

    // sort in and out going connections to avoid crossing connections
    auto const sortByNodePosition = [this](QtNodeId const& a, QtNodeId const& b){
        return m_model->nodeData(a, QtNodeRole::Position).toPointF().y() >
               m_model->nodeData(b, QtNodeRole::Position).toPointF().y();
    };

    std::sort(connectionsIn.begin(), connectionsIn.end(),
              [=](QtConnectionId const& a, QtConnectionId const& b){
        return sortByNodePosition(a.inNodeId, b.inNodeId);
    });
    std::sort(connectionsIn.begin(), connectionsIn.end(),
              [=](QtConnectionId const& a, QtConnectionId const& b){
        return sortByNodePosition(a.outNodeId, b.outNodeId);
    });

    // move input connections
    PortIndex index{0};
    for (QtConnectionId conId : connectionsIn)
    {
        // create connection in parent graph
        QtConnectionId newCon = conId;
        newCon.inNodeId = groupNode->id();
        newCon.inPortIndex = index;
        m_model->addConnection(newCon);

        // create connection in subgraph
        conId.outNodeId = inputProvider->id();
        conId.outPortIndex = index;
        groupModel->addConnection(conId);

        index++;
    }

    // move output connections
    index = PortIndex{0};
    for (QtConnectionId conId : connectionsOut)
    {
        // create connection in parent graph
        QtConnectionId newCon = conId;
        newCon.outNodeId = groupNode->id();
        newCon.outPortIndex = index;
        m_model->addConnection(newCon);

        // create connection in subgraph
        conId.inNodeId = outputProvider->id();
        conId.inPortIndex = index;
        groupModel->addConnection(conId);

        index++;
    }

    // move internal connections
    for (QtConnectionId const& conId : connectionsInternal)
    {
        groupModel->addConnection(conId);
    }
}
