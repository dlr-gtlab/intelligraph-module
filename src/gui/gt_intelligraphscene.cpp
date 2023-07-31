/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphscene.h"

#include "private/utils.h"
#include "gt_intelligraphnodeui.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphdatafactory.h"
#include "gt_intelligraphjsonadapter.h"
#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphmodeladapter.h"

#include "gt_application.h"
#include "gt_command.h"
#include "gt_datamodel.h"
#include "gt_qtutilities.h"
#include "gt_guiutilities.h"
#include "gt_icons.h"
#include "gt_inputdialog.h"
#include "gt_objectmemento.h"
#include "gt_objectfactory.h"

#include <gt_logging.h>

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

#include <QGraphicsItem>
#include <QClipboard>
#include <QApplication>

template <typename In, typename Out>
void findConnections(GtIntelliGraph& graph, In const& in, Out& out)
{
    out.reserve(in.size());

    for (auto conId : in)
    {
        if (auto* con = graph.findConnection(conId))
        {
            out.push_back(con);
        }
    }
}

template <typename In, typename Out>
void findNodes(GtIntelliGraph& graph, In const& in, Out& out, bool onlyDeletable = false)
{
    out.reserve(in.size());

    for (auto nodeId : in)
    {
        if (auto* node = graph.findNode(gt::ig::NodeId::fromValue(nodeId)))
        {
            if (!onlyDeletable || (onlyDeletable && node->objectFlags() & GtObject::UserDeletable))
            {
                out.push_back(node);
            }
        }
    }
}

struct SelectedItems
{
    QVector<QtNodes::NodeId> nodes;
    QVector<QtNodes::ConnectionId> connections;

    bool empty() const { return nodes.empty() && connections.empty(); }
};

SelectedItems
findSelectedItems(GtIntelliGraphScene& scene)
{
    auto  const& selected = scene.selectedItems();

    if (selected.empty()) return {};

    SelectedItems items;

    for (auto* item : selected)
    {
        if (auto* node = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item))
        {
            items.nodes << node->nodeId();
            continue;
        }
        if (auto* con = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item))
        {
            items.connections << con->connectionId();
            continue;
        }
    }
    return items;
}

template<typename T>
QList<T>
findItems(GtIntelliGraphScene& scene)
{
    QList<T> items;

    for (auto* item : scene.items())
    {
        if (auto* obj = qgraphicsitem_cast<T>(item))
        {
            items << obj;
        }
    }
    return items;
}

GtIntelliGraphScene::GtIntelliGraphScene(GtIntelliGraph& graph) :
    QtNodes::DataFlowGraphicsScene(*graph.makeModelAdapter()->graphModel()),
    m_data(&graph),
    m_model(static_cast<QtNodes::DataFlowGraphModel*>(&graphModel()))
{
    setParent(m_model);

    connect(this, &QtNodes::DataFlowGraphicsScene::nodeMoved,
            this, &GtIntelliGraphScene::onNodePositionChanged);
    connect(this, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &GtIntelliGraphScene::onNodeSelected);
    connect(this, &QtNodes::DataFlowGraphicsScene::nodeDoubleClicked,
            this, &GtIntelliGraphScene::onNodeDoubleClicked);
    connect(this, &QtNodes::DataFlowGraphicsScene::nodeContextMenu,
            this, &GtIntelliGraphScene::onNodeContextMenu);
    connect(this, &QtNodes::DataFlowGraphicsScene::portContextMenu,
            this, &GtIntelliGraphScene::onPortContextMenu);
    connect(this, &QtNodes::DataFlowGraphicsScene::widgetResized,
            this, &GtIntelliGraphScene::onWidgetResized);

//    connect(m_model, &QtNodes::DataFlowGraphModel::nodePositionUpdated,
//            this, &GtIntelliGraphScene::onNodePositionChanged);
}

void
GtIntelliGraphScene::deleteSelectedObjects()
{
    auto const& selected = ::findSelectedItems(*this);
    if (selected.empty()) return;

    GtObjectList objects;
    findConnections(*m_data, selected.connections, objects);
    findNodes(*m_data, selected.nodes, objects, true);

    gtDataModel->deleteFromModel(objects);
}

void
GtIntelliGraphScene::duplicateSelectedObjects()
{
    if (!copySelectedObjects()) return;

    pasteObjects();
}

bool
GtIntelliGraphScene::copySelectedObjects()
{
    auto selected = findSelectedItems(*this);
    if (selected.nodes.empty()) return false;

    // only duplicate internal connections
    auto iter = 0;
    foreach (auto conId, selected.connections)
    {
        if (!selected.nodes.contains(conId.inNodeId) ||
            !selected.nodes.contains(conId.outNodeId))
        {
            selected.connections.remove(iter);
            continue;
        }
        iter++;
    }

    QList<GtIntelliGraphNode const*> nodes;
    QList<GtIntelliGraphConnection const*> connections;
    findConnections(*m_data, selected.connections, connections);
    findNodes(*m_data, selected.nodes, nodes);

    // at least one node should be selected
    if (nodes.empty()) return false;

    QJsonDocument doc(gt::ig::toJson(nodes, connections));
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));

    return true;
}

void
GtIntelliGraphScene::pasteObjects()
{
    using namespace gt::ig;

    gtDebug().medium() << __FUNCTION__;

    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    auto doc = QJsonDocument::fromJson(text.toUtf8());
    if (doc.isNull()) return;

    // restore objects
    auto objects = gt::ig::fromJson(doc.object());
    if (!objects) return;

    // shift node positions
    constexpr QPointF offset{50, 50};

    for (auto& node : objects->nodes)
    {
        node->setPos(node->pos() + offset);
    }

    auto cmd = gtApp->startCommand(m_data, tr("Paste objects"));
    auto cleanup = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    // append objects
    auto newNodeIds = m_data->appendObjects(objects->nodes, objects->connections);

    // set selection
    auto nodes = findItems<QtNodes::NodeGraphicsObject*>(*this);
    auto iter = 0;
    foreach (auto* node, nodes)
    {
        auto nodeId = node->nodeId();
        if (!newNodeIds.contains(NodeId::fromValue(nodeId)))
        {
            nodes.removeAt(iter);
            continue;
        }
        iter++;
    }

    auto connections = findItems<QtNodes::ConnectionGraphicsObject*>(*this);
    iter = 0;
    foreach (auto* con, connections)
    {
        auto conId = con->connectionId();
        if (!newNodeIds.contains(NodeId::fromValue(conId.inNodeId)) ||
            !newNodeIds.contains(NodeId::fromValue(conId.outNodeId)))
        {
            connections.removeAt(iter);
            continue;
        }
        iter++;
    }

    clearSelection();
    for (auto* item : qAsConst(nodes)) item->setSelected(true);
    for (auto* item : qAsConst(connections)) item->setSelected(true);
}

void
GtIntelliGraphScene::onNodePositionChanged(QtNodes::NodeId nodeId)
{
    auto* node = m_data->findNode(gt::ig::NodeId::fromValue(nodeId));

    if (!node) return;

    auto position = m_model->nodeData(nodeId, QtNodes::NodeRole::Position);

    if (!position.isValid()) return;

    auto pos = position.toPointF();

    gtInfo().verbose()
        << tr("Updating node position to") << pos
        << gt::brackets(node->objectName());

    node->setPos(pos);
}

void
GtIntelliGraphScene::onNodeSelected(QtNodes::NodeId nodeId)
{
    if (auto* node = m_data->findNode(gt::ig::NodeId::fromValue(nodeId)))
    {
        emit gtApp->objectSelected(node);
    }
}

void
GtIntelliGraphScene::onNodeDoubleClicked(QtNodes::NodeId nodeId)
{
    if (auto* node = m_data->findNode(gt::ig::NodeId::fromValue(nodeId)))
    {
        gt::gui::handleObjectDoubleClick(*node);
    }
}

void
GtIntelliGraphScene::onWidgetResized(QtNodes::NodeId nodeId, QSize size)
{
    if (auto* node = m_data->findNode(gt::ig::NodeId::fromValue(nodeId)))
    {
        node->setSize(size);
    }
}

void
GtIntelliGraphScene::onPortContextMenu(QtNodes::NodeId nodeId,
                                       QtNodes::PortType type,
                                       QtNodes::PortIndex idx,
                                       QPointF pos)
{
    using PortType  = gt::ig::PortType;
    using PortIndex = gt::ig::PortIndex;

    auto* node = m_data->findNode(gt::ig::NodeId::fromValue(nodeId));
    if (!node) return;

    // create menu
    QMenu menu;

    QList<GtObjectUI*> const& uis = gtApp->objectUI(node);
    QVector<GtIntelliGraphNodeUI*> nodeUis;
    nodeUis.reserve(uis.size());
    for (auto* ui : uis)
    {
        if (auto* nodeUi = qobject_cast<GtIntelliGraphNodeUI*>(ui))
        {
            nodeUis.push_back(nodeUi);
        }
    }

    // add custom action
    QHash<QAction*, typename GtIgPortUIAction::ActionMethod> actions;

    for (auto* nodeUi : nodeUis)
    {
        for (auto const& actionData : nodeUi->portActions())
        {
            if (actionData.empty())
            {
                menu.addSeparator();
                continue;
            }

            if (actionData.visibilityMethod() &&
                !actionData.visibilityMethod()(node, static_cast<PortType>(type), PortIndex::fromValue(idx)))
            {
                continue;
            }

            auto* action = menu.addAction(actionData.text());
            action->setIcon(actionData.icon());

            if (actionData.verificationMethod() &&
                !actionData.verificationMethod()(node, static_cast<PortType>(type), PortIndex::fromValue(idx)))
            {
                action->setEnabled(false);
            }

            actions.insert(action, actionData.method());
        }
    }

    menu.addSeparator();

    QList<GtObject*> connections;
    findConnections(*m_data, m_model->connections(nodeId, type, idx), connections);

    QAction* deleteAct = menu.addAction(tr("Remove all connections"));
    deleteAct->setEnabled(!connections.empty());
    deleteAct->setIcon(gt::gui::icon::chainOff());

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == deleteAct)
    {
        gtDataModel->deleteFromModel(connections);
        return;
    }

    // call custom action
    if (auto action = actions.value(triggered))
    {
        action(node, static_cast<PortType>(type), PortIndex::fromValue(idx));
    }
}

void
GtIntelliGraphScene::onNodeContextMenu(QtNodes::NodeId nodeId, QPointF pos)
{
    // retrieve selected nodes
    auto selectedNodeIds = selectedNodes();

    bool allDeletable = std::all_of(selectedNodeIds.begin(),
                                    selectedNodeIds.end(),
                                    [this](QtNodes::NodeId id){
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
        auto id = gt::ig::NodeId::fromValue(selectedNodeIds.front());
        if (GtIntelliGraphNode* node = m_data->findNode(id))
        {
            menu.addSeparator();

            gt::gui::makeObjectContextMenu(menu, *node);
        }
        deleteAct->setVisible(false);
    }

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == groupAct)
    {
        return makeGroupNode(selectedNodeIds);
    }
    if (triggered == deleteAct)
    {
        return deleteNodes(selectedNodeIds);
    }
}

void
GtIntelliGraphScene::deleteNodes(const std::vector<QtNodes::NodeId>& nodeIds)
{
    QList<GtObject*> nodes;
    findNodes(*m_data, nodeIds, nodes);

    gtDataModel->deleteFromModel(nodes);
}

void
GtIntelliGraphScene::makeGroupNode(std::vector<QtNodes::NodeId> const& selectedNodeIds)
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

    for (QtNodes::NodeId nodeId : selectedNodeIds)
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
        QtNodes::NodeId nodeId = c.inNodeId;
        QtNodes::PortIndex port = c.inPortIndex;

        auto dtype = m_model->portData(nodeId, QtNodes::PortType::In, port, QtPortRole::DataType)
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
        QtNodes::NodeId nodeId = c.outNodeId;
        QtNodes::PortIndex port = c.outPortIndex;

        auto dtype = m_model->portData(nodeId, QtNodes::PortType::Out, port, QtPortRole::DataType)
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

    auto cmd = gtApp->startCommand(m_data, tr("Create group node '%1'").arg(groupNodeName));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    // create group node
    auto* groupNode = static_cast<GtIntelliGraph*>(m_data->appendNode(std::make_unique<GtIntelliGraph>()));
    if (!groupNode || !groupNode->findModelAdapter())
    {
        gtError() << tr("Failed to create group node! (Invalid group node)");
        return;
    }

    groupNode->setCaption(groupNodeName);

    // setup input/output provider
    groupNode->initGroupProviders();
    auto* inputProvider = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << tr("Failed to create group node! "
                        "(Invalid input or output provider)");
        gtError() << inputProvider << outputProvider;
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

    std::vector<GtIntelliGraphNode*> selectedNodes;
    findNodes(*m_data, selectedNodeIds, selectedNodes);

    if (selectedNodes.size() != selectedNodeIds.size())
    {
        gtError() << tr("Failed to create group node! "
                        "(Some nodes were not not found)");
        return;
    }

    QPolygonF selectionPoly;
    std::transform(selectedNodes.begin(), selectedNodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    // update node positions
    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    auto offset = QPointF{boundingRect.width() / 2, boundingRect.height() / 2};

    m_data->setNodePosition(groupNode, center);
    groupNode->setNodePosition(inputProvider, inputProvider->pos() - offset);
    groupNode->setNodePosition(outputProvider, outputProvider->pos() + offset);

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

        // udpate connections if node id has changed
        if (newId == oldId) continue;

        gtInfo().verbose() << "Updating node id from" << oldId << "to" << newId << "...";

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

    // remove old nodes and connections. COnnections must be deleted before
    // appending new connections
    qDeleteAll(selectedNodes);

    // sort in and out going connections to avoid crossing connections
    auto const sortByNodePosition = [this](QtNodes::NodeId const& a, QtNodes::NodeId const& b){
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
        m_data->appendConnection(std::make_unique<GtIntelliGraphConnection>(newCon));

        // create connection in subgraph
        conId.outNodeId = inputProvider->id();
        conId.outPortIndex = index;
        groupNode->appendConnection(std::make_unique<GtIntelliGraphConnection>(conId));

        index++;
    }

    // move output connections
    index = PortIndex{0};
    for (ConnectionId conId : connectionsOut)
    {
        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.outNodeId = groupNode->id();
        newCon.outPortIndex = index;
        m_data->appendConnection(std::make_unique<GtIntelliGraphConnection>(newCon));

        // create connection in subgraph
        conId.inNodeId = outputProvider->id();
        conId.inPortIndex = index;
        groupNode->appendConnection(std::make_unique<GtIntelliGraphConnection>(conId));

        index++;
    }

    // move internal connections
    for (ConnectionId const& conId : connectionsInternal)
    {
       groupNode->appendConnection(std::make_unique<GtIntelliGraphConnection>(conId));
    }
}
