/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/graphscene.h"

#include "intelli/connection.h"
#include "intelli/graphadaptermodel.h"
#include "intelli/graphexecmodel.h"
#include "intelli/nodedatafactory.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/gui/nodeui.h"
#include "intelli/private/utils.h"

#include <gt_application.h>
#include <gt_command.h>
#include <gt_datamodel.h>
#include <gt_qtutilities.h>
#include <gt_guiutilities.h>
#include <gt_icons.h>
#include <gt_inputdialog.h>
#include <gt_objectmemento.h>
#include <gt_objectfactory.h>

#include <gt_logging.h>

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

#include <QGraphicsItem>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QMenuBar>

using namespace intelli;

struct GraphScene::Impl
{

template <typename In, typename Out>
static void findConnections(Graph& graph, In const& in, Out& out)
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
static void findNodes(Graph& graph, In const& in, Out& out, bool onlyDeletable = false)
{
    out.reserve(in.size());

    for (auto nodeId : in)
    {
        if (auto* node = graph.findNode(NodeId::fromValue(nodeId)))
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
    QVector<NodeId> nodes;
    QVector<ConnectionId> connections;

    bool empty() const { return nodes.empty() && connections.empty(); }
};

static SelectedItems findSelectedItems(GraphScene& scene)
{
    auto  const& selected = scene.selectedItems();

    if (selected.empty()) return {};

    SelectedItems items;

    for (auto* item : selected)
    {
        if (auto* node = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item))
        {
            items.nodes << NodeId(node->nodeId());
            continue;
        }
        if (auto* con = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item))
        {
            items.connections << scene.adapterModel().convert(con->connectionId());
            continue;
        }
    }
    return items;
}

template<typename T>
static QList<T> findItems(GraphScene& scene)
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

};

GraphScene::GraphScene(Graph& graph) :
    QtNodes::BasicGraphicsScene(*new GraphAdapterModel(graph)),
    m_data(&graph)
{
    adapterModel().setParent(this);

    connect(this, &QtNodes::BasicGraphicsScene::nodeSelected,
            this, &GraphScene::onNodeSelected);
    connect(this, &QtNodes::BasicGraphicsScene::nodeDoubleClicked,
            this, &GraphScene::onNodeDoubleClicked);
    connect(this, &QtNodes::BasicGraphicsScene::nodeContextMenu,
            this, &GraphScene::onNodeContextMenu);
    connect(this, &QtNodes::BasicGraphicsScene::portContextMenu,
            this, &GraphScene::onPortContextMenu);
    connect(this, &QtNodes::BasicGraphicsScene::widgetResized,
            this, &GraphScene::onWidgetResized);
    connect(this, &QtNodes::BasicGraphicsScene::nodeClicked, this, [this](QtNodes::NodeId qnodeId){
        auto& model = adapterModel();
        model.commitPosition(NodeId(qnodeId));
        for (QtNodes::NodeId nodeId : selectedNodes())
        {
            model.commitPosition(NodeId(nodeId));
        }
    });
}

GraphScene::~GraphScene()
{
    gtDebug() << __FUNCTION__;
}

bool
GraphScene::autoEvaluate(bool enable)
{
    auto* model = m_data->makeMainExecutionModel();

    return model->autoEvaluate(enable);
}

bool
GraphScene::isAutoEvaluating()
{
    auto* model = m_data->makeMainExecutionModel();

    return model->isAutoEvaluating();
}

void
GraphScene::deleteSelectedObjects()
{
    auto const& selected = Impl::findSelectedItems(*this);
    if (selected.empty()) return;

    GtObjectList objects;
    Impl::findConnections(*m_data, selected.connections, objects);
    Impl::findNodes(*m_data, selected.nodes, objects, true);

    gtDataModel->deleteFromModel(objects);
}

void
GraphScene::duplicateSelectedObjects()
{
    if (!copySelectedObjects()) return;

    pasteObjects();
}

bool
GraphScene::copySelectedObjects()
{
    auto selected = Impl::findSelectedItems(*this);
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

    QList<Node const*> nodes;
    QList<Connection const*> connections;
    Impl::findConnections(*m_data, selected.connections, connections);
    Impl::findNodes(*m_data, selected.nodes, nodes);

#if 0
    // at least one node should be selected
    if (nodes.empty()) return false;

    QJsonDocument doc(toJson(nodes, connections));
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));

    return true;
#else
    return false;
#endif
}

void
GraphScene::pasteObjects()
{
    using namespace intelli;

    gtDebug().medium() << __FUNCTION__;

#if 0
    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    auto doc = QJsonDocument::fromJson(text.toUtf8());
    if (doc.isNull()) return;

    // restore objects
    auto objects = fromJson(doc.object());
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
    if (newNodeIds.size() != objects->nodes.size())
    {
        gtWarning() << tr("Pasting selection failed!");
        return;
    }

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
#endif
}

void
GraphScene::keyPressEvent(QKeyEvent* event)
{
    QVector<Node*> nodes;
    Impl::findNodes(*m_data, selectedNodes(), nodes);

    if (nodes.size() != 1) return QtNodes::BasicGraphicsScene::keyPressEvent(event);

    Node* node = nodes.front();
    assert(event);
    assert(node);

    event->setAccepted(false);

    gt::gui::handleObjectKeyEvent(*event, *nodes.front());

    if (!event->isAccepted()) QtNodes::BasicGraphicsScene::keyPressEvent(event);
}

void
GraphScene::onNodeSelected(QtNodes::NodeId nodeId)
{
    if (auto* node = m_data->findNode(NodeId::fromValue(nodeId)))
    {
        emit gtApp->objectSelected(node);
    }
}

void
GraphScene::onNodeDoubleClicked(QtNodes::NodeId nodeId)
{
    if (auto* node = m_data->findNode(NodeId::fromValue(nodeId)))
    {
        gt::gui::handleObjectDoubleClick(*node);
    }
}

void
GraphScene::onWidgetResized(QtNodes::NodeId nodeId, QSize size)
{
    if (auto* node = m_data->findNode(NodeId::fromValue(nodeId)))
    {
        node->setSize(size);
    }
}

void
GraphScene::onPortContextMenu(QtNodes::NodeId qnodeId,
                              QtNodes::PortType qtype,
                              QtNodes::PortIndex qidx,
                              QPointF pos)
{
    using PortType  = PortType;
    using PortIndex = PortIndex;

    NodeId nodeId(qnodeId);
    PortType type(::convert(qtype));
    PortIndex idx(qidx);

    auto* node = m_data->findNode(NodeId::fromValue(nodeId));
    if (!node) return;

    clearSelection();

    // create menu
    QMenu menu;

    QList<GtObjectUI*> const& uis = gtApp->objectUI(node);
    QVector<NodeUI*> nodeUis;
    nodeUis.reserve(uis.size());
    for (auto* ui : uis)
    {
        if (auto* nodeUi = qobject_cast<NodeUI*>(ui))
        {
            nodeUis.push_back(nodeUi);
        }
    }

    // add custom action
    QHash<QAction*, typename PortUIAction::ActionMethod> actions;

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

    PortId portId = m_data->portId(nodeId, type, idx);

    QList<GtObject*> connections;
    Impl::findConnections(*m_data, m_data->findConnections(nodeId, portId), connections);

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
GraphScene::onNodeContextMenu(QtNodes::NodeId qnodeId, QPointF pos)
{
    NodeId nodeId(qnodeId);

    auto nodeItem = nodeGraphicsObject(nodeId);
    if (!nodeItem) return;

    if (!nodeItem->isSelected()) clearSelection();
    nodeItem->setSelected(true);

    // retrieve selected nodes
    auto selectedNodeIds = selectedNodes();

    bool allDeletable = std::all_of(selectedNodeIds.begin(),
                                    selectedNodeIds.end(),
                                    [this](QtNodes::NodeId id){
        return adapterModel().nodeFlags(id) & QtNodes::Deletable;
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
        auto id = NodeId::fromValue(selectedNodeIds.front());
        if (Node* node = m_data->findNode(id))
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
GraphScene::deleteNodes(const std::vector<QtNodes::NodeId>& nodeIds)
{
    QList<GtObject*> nodes;
    Impl::findNodes(*m_data, nodeIds, nodes);

    gtDataModel->deleteFromModel(nodes);
}

void
GraphScene::makeGroupNode(std::vector<QtNodes::NodeId> const& selectedNodeIds)
{
    // get new node name
    GtInputDialog dialog(GtInputDialog::TextInput);
    dialog.setWindowTitle(tr("New Node Caption"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(tr("Enter a new caption for the grouped nodes"));
    dialog.setInitialTextValue(QStringLiteral("Graph"));
    if (!dialog.exec()) return;

    QString const& groupNodeName = dialog.textValue();

    // find input/output connections and connections to move
    std::vector<ConnectionId> connectionsInternal, connectionsIn, connectionsOut;

    for (QtNodes::NodeId qnodeId : selectedNodeIds)
    {
        NodeId nodeId(qnodeId);
        if (!m_data->findNode(nodeId)) continue;

        // check connections
        for (ConnectionId conId : m_data->findConnections(nodeId))
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

//    // sort in and out going connections to avoid crossing connections
//    auto const sortByPortIndex = [](PortIndex a, PortIndex b){
//        return a < b;
//    };

//    std::sort(connectionsIn.begin(), connectionsIn.end(),
//              [=](ConnectionId const& a, ConnectionId const& b){
//                  return sortByPortIndex(a.inPortIndex, b.inPortIndex);
//              });
//    std::sort(connectionsOut.begin(), connectionsOut.end(),
//              [=](ConnectionId const& a, ConnectionId const& b){
//                  return sortByPortIndex(a.outPortIndex, b.outPortIndex);
//              });

    // find datatype for input provider
    std::vector<QString> dtypeIn;
    for (ConnectionId c : connectionsIn)
    {
        auto* node = m_data->findNode(c.inNodeId);
        assert(node);
        auto* port = node->port(c.inPort);
        assert(port);

        if (!NodeDataFactory::instance().knownClass(port->typeId))
        {
            gtError() << tr("Failed to create group node! "
                            "(Unkown node datatype '%1', id: %2, port: %3)")
                             .arg(port->typeId, node->caption(), toString(*port));
        }

        dtypeIn.push_back(port->typeId);
    }

    // find datatypes for output provider
    std::vector<QString> dtypeOut;
    for (ConnectionId c : connectionsOut)
    {
        auto* node = m_data->findNode(c.outNodeId);
        assert(node);
        auto* port = node->port(c.outPort);
        assert(port);

        if (!NodeDataFactory::instance().knownClass(port->typeId))
        {
            gtError() << tr("Failed to create group node! "
                            "(Unkown node datatype '%1', id: %2, port: %3)")
                             .arg(port->typeId, node->caption(), toString(*port));
        }

        dtypeOut.push_back(port->typeId);
    }

    auto cmd = gtApp->startCommand(m_data, tr("Create group node '%1'").arg(groupNodeName));
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    // create group node
    auto* groupNode = static_cast<Graph*>(m_data->appendNode(std::make_unique<Graph>()));
    if (!groupNode)
    {
        gtError() << tr("Failed to create group node! (Invalid group node)");
        return;
    }

    groupNode->setCaption(groupNodeName);

    // setup input/output provider
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

    std::vector<Node*> selectedNodes;
    Impl::findNodes(*m_data, selectedNodeIds, selectedNodes);

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

    groupNode->setPos(center);
    inputProvider->setPos(inputProvider->pos() - offset);
    outputProvider->setPos(outputProvider->pos() + offset);

    // move selected nodes
    for (auto* node : selectedNodes)
    {
        auto newNode = gt::unique_qobject_cast<Node>(
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

    // remove old nodes and connections. Connections must be deleted before
    // appending new connections
    qDeleteAll(selectedNodes);

    // move input connections
    PortIndex index{0};
    for (ConnectionId conId : connectionsIn)
    {
        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.inNodeId = groupNode->id();
        newCon.inPort   = groupNode->portId(PortType::In, index);
        m_data->appendConnection(std::make_unique<Connection>(newCon));

        // create connection in subgraph
        conId.outNodeId = inputProvider->id();
        conId.outPort   = inputProvider->portId(PortType::Out, index);
        groupNode->appendConnection(std::make_unique<Connection>(conId));

        index++;
    }

    // move output connections
    index = PortIndex{0};
    for (ConnectionId conId : connectionsOut)
    {
        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.outNodeId = groupNode->id();
        newCon.inPort    = groupNode->portId(PortType::Out, index);
        m_data->appendConnection(std::make_unique<Connection>(newCon));

        // create connection in subgraph
        conId.inNodeId = outputProvider->id();
        conId.outPort  = outputProvider->portId(PortType::In, index);
        groupNode->appendConnection(std::make_unique<Connection>(conId));

        index++;
    }

    // move internal connections
    for (ConnectionId const& conId : connectionsInternal)
    {
        groupNode->appendConnection(std::make_unique<Connection>(conId));
    }
}

GraphAdapterModel&
GraphScene::adapterModel()
{
    auto* tmp = static_cast<GraphAdapterModel*>(&graphModel());
    assert(qobject_cast<GraphAdapterModel*>(&graphModel()));
    return *tmp;
}
