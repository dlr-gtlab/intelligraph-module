/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/graphscene.h"

#include <intelli/graph.h>
#include "intelli/connection.h"
#include "intelli/nodefactory.h"
#include "intelli/nodedatafactory.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/gui/nodeui.h"
#include "intelli/gui/nodegeometry.h"
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/connectionobject.h>
#include <intelli/private/utils.h>

#include <gt_application.h>
#include <gt_command.h>
#include <gt_datamodel.h>
#include <gt_qtutilities.h>
#include <gt_guiutilities.h>
#include <gt_icons.h>
#include <gt_inputdialog.h>
#include <gt_objectmemento.h>
#include <gt_objectio.h>
#include <gt_objectfactory.h>

#include <gt_logging.h>

#include <QMenu>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QElapsedTimer>
#include <QTimer>
#include <QWidgetAction>
#include <QMenuBar>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QVarLengthArray>

constexpr QPointF s_connection_distance{5, 5};

using namespace intelli;

template <typename T>
inline auto makeCopy(T& obj) {
    std::unique_ptr<GtObject> tmp{obj.copy()};
    return gt::unique_qobject_cast<std::remove_const_t<T>>(std::move(tmp));
};

struct GraphScene::Impl
{

/// helper struct to collect selected nodes and connections
struct SelectedItems
{
    QVector<NodeGraphicsObject*> nodes;
    QVector<ConnectionGraphicsObject*> connections;

    bool empty() const { return nodes.empty() && connections.empty(); }
};

/// enum to filter selection
enum SelectionFilter
{
    NoFilter,
    NodesOnly,
    ConnectionsOnly
};

/**
 * @brief Helper function to find all selected nodes and connections. If a
 * filter was supplied, only Nodes or only Connections are collected.
 * @param scene Scene object (this)
 * @param filter Filter for selection
 * @return Selected items
 */
static SelectedItems
findSelectedItems(GraphScene& scene, SelectionFilter filter = NoFilter)
{
    auto const& selected = scene.selectedItems();

    SelectedItems items;
    for (auto* item : selected)
    {
        if (filter != ConnectionsOnly)
        if (auto* node = qgraphicsitem_cast<NodeGraphicsObject*>(item))
        {
            items.nodes << node;
            continue;
        }
        if (filter != NodesOnly)
        if (auto* con = qgraphicsitem_cast<ConnectionGraphicsObject*>(item))
        {
            items.connections << con;
            continue;
        }
    }

    return items;
}

/**
 * @brief Find all items of type T
 * @param scene Scene object (this)
 * @return All graphic items of type T
 */
template<typename T>
static QVector<T>
findItems(GraphScene& scene)
{
    QVector<T> items;

    auto const& sceneItems = scene.items();
    for (auto* item : sceneItems)
    {
        if (auto* obj = qgraphicsitem_cast<T>(item))
        {
            items << obj;
        }
    }
    return items;
}

static void
highlightCompatibleNodes(GraphScene& scene,
                         Node& sourceNode,
                         Node::PortInfo const& sourcePort)
{
    NodeId sourceNodeId = sourceNode.id();
    PortId sourcePortId = sourcePort.id();
    PortType type = sourceNode.portType(sourcePortId);
    assert(type != PortType::NoType);

    // "deemphasize" all connections
    for (auto& con : scene.m_connections)
    {
        con.object->makeInactive(true);
    }

    // find nodes that can potentially recieve connection
    // -> all nodes that we do not depend on/that do not depend on us
    QVector<NodeId> targets;
    auto nodeIds = scene.m_graph->nodeIds();
    auto dependencies = type == PortType::Out ?
                            scene.m_graph->findDependencies(sourceNodeId) :
                            scene.m_graph->findDependentNodes(sourceNodeId);

    std::sort(nodeIds.begin(), nodeIds.end());
    std::sort(dependencies.begin(), dependencies.end());

    std::set_difference(nodeIds.begin(), nodeIds.end(),
                        dependencies.begin(), dependencies.end(),
                        std::back_inserter(targets));

    targets.removeOne(sourceNodeId);

    // frist "unhilight" all nodes
    for (NodeId nodeId : qAsConst(nodeIds))
    {
        NodeGraphicsObject* target = scene.nodeObject(nodeId);
        assert(target);
        target->highlights().setAsIncompatible();
    }
    // then highlight all nodes that are compatible
    for (NodeId nodeId : qAsConst(targets))
    {
        NodeGraphicsObject* target = scene.nodeObject(nodeId);
        assert(target);
        target->highlights().setCompatiblePorts(sourcePort.typeId, invert(type));
    }

    // override source port
    NodeGraphicsObject* source = scene.nodeObject(sourceNodeId);
    assert(source);
    source->highlights().setPortAsCompatible(sourcePortId);
}

static void
clearHighlights(GraphScene& scene)
{
    // clear target nodes
    for (auto& entry : scene.m_nodes)
    {
        assert(entry.object);
        entry.object->highlights().clear();
    }
    for (auto& con : scene.m_connections)
    {
        con.object->makeInactive(false);
    }
}

/**
 * @brief Instantiates a draft connection. Appends the object to the scene and
 * sets both end points and grabs the mouse.
 * @param scene Scene object (this)
 * @param sourceObject Object that the draft connection starts from (or ends at)
 * @param sourceType Whether this connection is starts at an output or input
 * @param sourcePortId Port that the draft connection starts from (or ends at)
 * @return Pointer to draft connection.
 */
static ConnectionGraphicsObject*
instantiateDraftConnection(GraphScene& scene,
                           NodeGraphicsObject& sourceObject,
                           PortType sourceType,
                           PortId sourcePortId)
{
    assert(!scene.m_draftConnection);
    assert(sourcePortId.isValid());

    NodeId sourceNodeId = sourceObject.nodeId();

    // dummy connection (respective end point is not connected)
    ConnectionId draftConId{
        sourceNodeId,
        sourcePortId,
        invalid<NodeId>(),
        invalid<PortId>()
    };

    auto& sourceNode = sourceObject.node();

    auto* sourcePort = sourceNode.port(sourcePortId);
    assert(sourcePort);

    // typeId
    TypeId dummy;
    TypeId* outType = &sourcePort->typeId;
    TypeId* inType  = &dummy;

    if (sourceType == PortType::In)
    {
        draftConId.reverse();
        std::swap(outType, inType);
    }

    assert(draftConId.draftType() == sourceType);

    auto entity = make_volatile<ConnectionGraphicsObject>(draftConId, *outType, *inType);
    entity->setConnectionShape(scene.m_connectionShape);
    scene.addItem(entity);

    // move respective start point of connection
    scene.moveConnectionPoint(*entity, sourceType);
    // move respective end point to start point and grab mouse
    entity->setEndPoint(invert(sourceType), entity->endPoint(sourceType));
    entity->grabMouse();

    scene.m_draftConnection = std::move(entity);

    highlightCompatibleNodes(scene, sourceNode, *sourcePort);

    return scene.m_draftConnection.get();
};

static void
makeDraftConnection(GraphScene& scene,
                    NodeGraphicsObject& object,
                    ConnectionId conId)
{
    auto const getEndPoint = [&scene](ConnectionId conId, PortType type){
        ConnectionGraphicsObject* oldCon = scene.connectionObject(conId);
        assert(oldCon);
        return oldCon->endPoint(type);
    };

    assert(!scene.m_draftConnection);
    assert(conId.isValid());
    assert(conId.inNodeId == object.nodeId());

    // this function is only called if an ingoing connection was disconnected
    constexpr PortType type = PortType::In;

    QPointF oldEndPoint = getEndPoint(conId, type);

    // delete old connection
    bool success = gtDataModel->deleteFromModel(scene.graph().findConnection(conId));
    assert(success);

    auto outNode = scene.nodeObject(conId.outNodeId);
    assert(outNode);

    // make draft connection form outgoing node
    auto* draft = Impl::instantiateDraftConnection(scene,
                                                   *outNode,
                                                   invert(type),
                                                   conId.outPort);

    // move initial end position of draft connection
    assert(draft);
    draft->setEndPoint(type, oldEndPoint);
}


/**
 * @brief Updates the connection's end point that the specified port type
 * refers to.
 * @param object Connection object to update
 * @param node Node object that the port belongs to
 * @param type Port type of the end point and target port index
 * @param portIdx Target port index
 */
static void
moveConnectionPoint(ConnectionGraphicsObject& object,
                    NodeGraphicsObject& node,
                    PortType type,
                    PortIndex portIdx)
{
    assert(portIdx != invalid<PortIndex>());

    auto const& geometry = node.geometry();

    QRectF portRect = geometry.portRect(type, portIdx);
    QPointF nodePos = node.sceneTransform().map(portRect.center());

    QPointF connectionPos = object.sceneTransform().inverted().map(nodePos);

    object.setEndPoint(type, connectionPos);
}

/**
 * @brief Overlaod that accepts a port id
 * @param object Connection object to update
 * @param node Node object that the port belongs to
 * @param type Port type of the end point and target port
 * @param port Target port
 */
static void
moveConnectionPoint(ConnectionGraphicsObject& object,
                    NodeGraphicsObject& node,
                    PortType type,
                    PortId port)
{
    assert(port != invalid<PortId>());

    PortIndex portIdx = node.node().portIndex(type, port);

    moveConnectionPoint(object, node, type, portIdx);
}

}; // struct Impl

GraphScene::GraphScene(Graph& graph) :
    m_graph(&graph),
    m_sceneData(std::make_unique<GraphSceneData>())
{
    // instantiate objects
    auto const& nodes = m_graph->nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }
    auto const& connections = m_graph->connections();
    for (auto* con : connections)
    {
        onConnectionAppended(con);
    }

    connect(m_graph, &Graph::nodeAppended, this, &GraphScene::onNodeAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::childNodeAboutToBeDeleted, this, &GraphScene::onNodeDeleted, Qt::DirectConnection);

    connect(m_graph, &Graph::connectionAppended, this, &GraphScene::onConnectionAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::connectionDeleted, this, &GraphScene::onConnectionDeleted, Qt::DirectConnection);

    setParent(&graph);
}

GraphScene::~GraphScene() = default;

Graph&
GraphScene::graph()
{
    assert(m_graph);
    return *m_graph;
}

Graph const&
GraphScene::graph() const
{
    return const_cast<GraphScene*>(this)->graph();
}

GraphSceneData const&
GraphScene::sceneData() const
{
    return *m_sceneData;
}

void
GraphScene::setGridSize(double gridSize)
{
    m_sceneData->gridSize = gridSize;
}

void
GraphScene::setSnapToGrid(bool enable)
{
    if (enable == m_sceneData->snapToGrid) return;

    m_sceneData->snapToGrid = enable;
    emit snapToGridChanged();
}

bool
GraphScene::snapToGrid() const
{
    return m_sceneData->snapToGrid;
}

void
GraphScene::setConnectionShape(ConnectionShape shape)
{
    if (shape == m_connectionShape) return;

    m_connectionShape = shape;
    if (m_draftConnection)
    {
        m_draftConnection->setConnectionShape(shape);
    }
    for (auto& con : m_connections)
    {
        con.object->setConnectionShape(shape);
    }
    emit connectionShapeChanged();
}

ConnectionShape
GraphScene::connectionShape() const
{
    return m_connectionShape;
}

NodeGraphicsObject*
GraphScene::nodeObject(NodeId nodeId)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
                             [nodeId](NodeEntry const& e){
        return e.nodeId == nodeId;
    });
    if (iter == m_nodes.end()) return nullptr;

    return iter->object;
}

NodeGraphicsObject const*
GraphScene::nodeObject(NodeId nodeId) const
{
    return const_cast<GraphScene*>(this)->nodeObject(nodeId);
}

ConnectionGraphicsObject*
GraphScene::connectionObject(ConnectionId conId)
{
    auto iter = std::find_if(m_connections.begin(), m_connections.end(),
                             [conId](ConnectionEntry const& e){
        return e.conId == conId;
    });
    if (iter == m_connections.end()) return nullptr;

    return iter->object;
}

ConnectionGraphicsObject const*
GraphScene::connectionObject(ConnectionId conId) const
{
    return const_cast<GraphScene*>(this)->connectionObject(conId);
}

QMenu*
GraphScene::createSceneMenu(QPointF scenePos)
{
// (adapted)
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    auto* menu = new QMenu;

    // Add filterbox to the context menu
    auto* txtBox = new QLineEdit(menu);
    txtBox->setPlaceholderText(QStringLiteral(" Filter"));
    txtBox->setClearButtonEnabled(true);

    // set the focus to allow text inputs
    QTimer::singleShot(0, txtBox, SIGNAL(setFocus()));

    auto* txtBoxAction = new QWidgetAction(menu);
    txtBoxAction->setDefaultWidget(txtBox);

    // 1.
    menu->addAction(txtBoxAction);

    // Add result treeview to the context menu
    QTreeWidget* treeView = new QTreeWidget(menu);
    treeView->header()->close();

    auto* treeViewAction = new QWidgetAction(menu);
    treeViewAction->setDefaultWidget(treeView);

    // 2.
    menu->addAction(treeViewAction);

    auto const& factory = NodeFactory::instance();

    auto cats = factory.registeredCategories();
    cats.sort();

    for (QString const& cat : qAsConst(cats))
    {
        if (cat.isEmpty()) continue;

        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }

    auto nodes = factory.registeredNodes();
    nodes.sort();

    for (QString const& node : qAsConst(nodes))
    {
        auto parents = treeView->findItems(factory.nodeCategory(node), Qt::MatchExactly);

        if (parents.empty()) continue;

        auto item = new QTreeWidgetItem(parents.first());
        item->setText(0, factory.nodeModelName(node));
        item->setWhatsThis(0, node); // store class name of node
    }

    treeView->expandAll();

    auto onClicked = [this, &factory, menu, scenePos](QTreeWidgetItem* item, int) {
        item->setExpanded(true);

        if (!(item->flags() & (Qt::ItemIsSelectable))) return;

        auto node = factory.makeNode(item->whatsThis(0));
        if (!node)
        {
            gtWarning() << tr("Failed to create new node of type %1").arg(item->text(0));
            return;
        }
        node->setPos(scenePos);

        auto cmd = gtApp->makeCommand(m_graph, tr("Append node '%1'").arg(node->caption()));
        Q_UNUSED(cmd);

        m_graph->appendNode(std::move(node));

        menu->close();
    };
    connect(treeView, &QTreeWidget::itemClicked, menu, onClicked);
    connect(treeView, &QTreeWidget::itemActivated, menu, onClicked);

    //Setup filtering
    connect(txtBox, &QLineEdit::textChanged, treeView, [treeView](QString const& text) {
        QTreeWidgetItemIterator categoryIt(treeView, QTreeWidgetItemIterator::HasChildren);

        while (*categoryIt) (*categoryIt++)->setHidden(true);

        QTreeWidgetItemIterator it(treeView, QTreeWidgetItemIterator::NoChildren);
        while (*it)
        {
            QString const& modelName = (*it)->text(0);
            bool match = (modelName.contains(text, Qt::CaseInsensitive));
            (*it)->setHidden(!match);
            if (match) {
                auto* parent = (*it)->parent();
                while (parent)
                {
                    parent->setHidden(false);
                    parent = parent->parent();
                }
            }
            ++it;
        }
    });

    return menu;
// SPDX-SnippetEnd
}

void
GraphScene::alignObjectsToGrid()
{
    for (NodeEntry& entry : m_nodes)
    {
        auto& object = entry.object;
        assert(object);
        object->setPos(quantize(object->pos(), m_sceneData->gridSize));
        object->commitPosition();
        moveConnections(object);
    }
}

void
GraphScene::deleteSelectedObjects()
{
    auto const& selected = Impl::findSelectedItems(*this);
    if (selected.empty()) return;

    GtObjectList objects;
    std::transform(selected.connections.begin(), selected.connections.end(), std::back_inserter(objects), [this](ConnectionGraphicsObject* o){ return m_graph->findConnection(o->connectionId()); });
    std::transform(selected.nodes.begin(), selected.nodes.end(), std::back_inserter(objects), [](NodeGraphicsObject* o){ return &o->node(); });

    auto modifyCmd = m_graph->modify();
    Q_UNUSED(modifyCmd);

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
    auto cleanup = gt::finally([](){
        QApplication::clipboard()->setText(QString{});
    });

    auto selected = Impl::findSelectedItems(*this);
    if (selected.nodes.empty()) return false;

    // only duplicate internal connections
    auto const containsNode = [&selected](NodeId nodeId){
        return std::find_if(selected.nodes.begin(), selected.nodes.end(), [nodeId](NodeGraphicsObject* o){
            return o->nodeId() == nodeId;
        }) != selected.nodes.end();
    };

    auto const isExternalConnection = [containsNode](ConnectionGraphicsObject const* o){
        auto con = o->connectionId();
        return !containsNode(con.inNodeId) || !containsNode(con.outNodeId);
    };

    selected.connections.erase(
        std::remove_if(selected.connections.begin(),
                       selected.connections.end(),
                       isExternalConnection),
        selected.connections.end());

    // remove unqiue nodes
    auto const isUnique = [](NodeGraphicsObject const* o){
        return o->node().nodeFlags() & NodeFlag::Unique;
    };

    selected.nodes.erase(
        std::remove_if(selected.nodes.begin(),
                       selected.nodes.end(),
                       isUnique),
        selected.nodes.end());

    // at least one node should be selected
    if (selected.nodes.empty()) return false;

    // append nodes and connections to dummy graph
    Graph dummy;
    for (auto* o : qAsConst(selected.nodes))
    {
        dummy.appendNode(makeCopy(o->node()));
    }
    for (auto* o : qAsConst(selected.connections))
    {
        dummy.appendConnection(std::make_unique<Connection>(o->connectionId()));
    }

    QApplication::clipboard()->setText(dummy.toMemento().toByteArray());
    cleanup.clear();
    return true;
}

void
GraphScene::pasteObjects()
{
    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    // restore objects
    GtObjectMemento mem(text.toUtf8());
    if (mem.isNull()) return;

    auto dummy = gt::unique_qobject_cast<Graph>(mem.toObject(*gtObjectFactory));
    if (!dummy) return;

    // regenerate uuids
    dummy->newUuid(true);
    dummy->resetGlobalConnectionModel();

    auto& conModel = dummy->connectionModel();
    if (conModel.empty()) return;

    auto nodes = dummy->nodes();
    bool hasConnections = conModel.hasConnections();

    // shift node positions
    constexpr QPointF offset{50, 50};

    for (auto& entry : conModel)
    {
        Node* node = entry.node;
        node->setPos(node->pos() + offset);
    }

    auto cmd = gtApp->makeCommand(m_graph, tr("Paste objects"));
    Q_UNUSED(cmd);

    // append objects
    bool success = dummy->moveNodesAndConnections(nodes, *m_graph);
    if (!success)
    {
        gtWarning() << tr("Pasting selection failed!");

        // restore previous state
        cmd.finalize();
        gtApp->undoStack()->undo();
        return;
    }

    // find affected graphics objects and set selection
    clearSelection();

    auto nodeObjects = Impl::findItems<NodeGraphicsObject*>(*this);
    for (auto* nodeObj : qAsConst(nodeObjects))
    {
        auto iter = std::find_if(nodes.begin(), nodes.end(),
                                 [nodeObj](Node* node){ return node->id() == nodeObj->nodeId(); });
        if (iter != nodes.end())
        {
            nodeObj->setSelected(true);
        }
    }

    if (!hasConnections) return;

    auto connectionObjects = Impl::findItems<ConnectionGraphicsObject*>(*this);
    for (auto* conObj : qAsConst(connectionObjects))
    {
        auto conId = conObj->connectionId();
        auto iterIn  = std::find_if(nodes.begin(), nodes.end(),
                                   [inId = conId.inNodeId](Node* node){ return node->id() == inId; });
        auto iterOut = std::find_if(nodes.begin(), nodes.end(),
                                    [outId = conId.outNodeId](Node* node){ return node->id() == outId; });
        if (iterIn != nodes.end() && iterOut != nodes.end())
        {
            conObj->setSelected(true);
        }
    }
}

void
GraphScene::keyPressEvent(QKeyEvent* event)
{
    // perform keyevent on node
    auto selected = Impl::findSelectedItems(*this, Impl::NodesOnly);

    if (selected.nodes.size() != 1) return QGraphicsScene::keyPressEvent(event);

    NodeGraphicsObject* o = selected.nodes.front();

    assert(event);
    event->setAccepted(false);

    gt::gui::handleObjectKeyEvent(*event, o->node());

    if (!event->isAccepted()) QGraphicsScene::keyPressEvent(event);
}

void
GraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsScene::mousePressEvent(event);
}

void
GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_draftConnection)
    {
        // snap to nearest possible port
        ConnectionId conId = m_draftConnection->connectionId();
        PortType targetType = invert(conId.draftType());
        bool reverse = conId.inNodeId.isValid();
        if (reverse) conId.reverse();

        QPointF pos = event->scenePos();

        QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

        auto const& items = this->items(rect);
        for (auto* item : items)
        {
            auto* object = qgraphicsitem_cast<NodeGraphicsObject*>(item);
            if (!object) continue;

            auto hit = object->geometry().portHit(object->mapFromScene(rect).boundingRect());
            if (!hit) continue;

            conId.inNodeId = object->nodeId();
            conId.inPort   = hit.port;
            assert(conId.isValid());

            if (reverse) conId.reverse();

            if (!m_graph->canAppendConnections(conId)) continue;

            Impl::moveConnectionPoint(*m_draftConnection, *object, hit.type, hit.port);
            return event->accept();
        }

        m_draftConnection->setEndPoint(targetType, event->scenePos());
        return event->accept();
    }

    return QGraphicsScene::mouseMoveEvent(event);
}

void
GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_draftConnection)
    {
        Impl::clearHighlights(*this);

        ConnectionId conId = m_draftConnection->connectionId();
        bool reverse = conId.inNodeId.isValid();
        if (reverse) conId.reverse();

        // remove draft connection
        m_draftConnection->ungrabMouse();
        m_draftConnection.reset();

        QPointF pos = event->scenePos();

        QRectF rect{pos - s_connection_distance, pos + s_connection_distance};

        // find node to connect to
        auto const& items = this->items(rect);
        for (auto* item : items)
        {
            auto* object = qgraphicsitem_cast<NodeGraphicsObject*>(item);
            if (!object) continue;

            auto hit = object->geometry().portHit(object->mapFromScene(rect).boundingRect());
            if (!hit) continue;

            conId.inNodeId = object->nodeId();
            conId.inPort   = hit.port;
            assert(conId.isValid());

            if (reverse) conId.reverse();

            if (!m_graph->canAppendConnections(conId)) continue;

            auto cmd = gtApp->makeCommand(m_graph, tr("Append %1").arg(toString(conId)));
            Q_UNUSED(cmd);

            m_graph->appendConnection(std::make_unique<Connection>(conId));
            break;
        }
        return event->accept();
    }

    return QGraphicsScene::mouseReleaseEvent(event);
}

void
GraphScene::onPortContextMenu(NodeGraphicsObject* object, PortId port, QPointF pos)
{
    using PortType  = PortType;
    using PortIndex = PortIndex;

    assert(object);

    auto* node = &object->node();

    clearSelection();

    PortType type = node->portType(port);
    PortIndex idx = node->portIndex(type, port);

    if (idx == invalid<PortIndex>()) return;

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
                !actionData.visibilityMethod()(node, type, idx))
            {
                continue;
            }

            auto* action = menu.addAction(actionData.text());
            action->setIcon(actionData.icon());

            if (actionData.verificationMethod() &&
                !actionData.verificationMethod()(node, type, idx))
            {
                action->setEnabled(false);
            }

            actions.insert(action, actionData.method());
        }
    }

    menu.addSeparator();

    auto& conModel = m_graph->connectionModel();
    auto connections = conModel.iterateConnections(node->id(), port);

    QAction* deleteAction = menu.addAction(tr("Remove all connections"));
    deleteAction->setEnabled(!connections.empty());
    deleteAction->setIcon(gt::gui::icon::chainOff());

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == deleteAction)
    {
        QList<GtObject*> objects;
        std::transform(connections.begin(), connections.end(),
                       std::back_inserter(objects),
                       [this](ConnectionId conId){
            return m_graph->findConnection(conId);
        });
        gtDataModel->deleteFromModel(objects);
        return;
    }

    // call custom action
    if (auto action = actions.value(triggered))
    {
        action(node, static_cast<PortType>(type), PortIndex::fromValue(idx));
    }
}

void
GraphScene::onNodeContextMenu(NodeGraphicsObject* object, QPointF pos)
{
    assert(object);

    auto* node = &object->node();

    if (!object->isSelected()) clearSelection();
    object->setSelected(true);

    // retrieve selected nodes
    auto selected = Impl::findSelectedItems(*this, Impl::NodesOnly);
    // selection should not be empty
    assert(!selected.nodes.empty());

    bool allDeletable = std::all_of(selected.nodes.begin(),
                                    selected.nodes.end(),
                                    [](NodeGraphicsObject* o){
        return o->node().objectFlags() & GtObject::ObjectFlag::UserDeletable;
    });

    // create menu
    QMenu menu;

    QAction* groupAction = menu.addAction(tr("Group selected Nodes"));
    groupAction->setIcon(gt::gui::icon::select());
    groupAction->setEnabled(allDeletable);
    groupAction->setVisible(!selected.nodes.empty());

    menu.addSeparator();

    QAction* deleteAction = menu.addAction(tr("Delete selected Nodes"));
    deleteAction->setIcon(gt::gui::icon::delete_());
    deleteAction->setEnabled(allDeletable);

    // add custom object menu
    if (selected.nodes.size() == 1)
    {
        menu.addSeparator();
        gt::gui::makeObjectContextMenu(menu, *node);
        deleteAction->setVisible(false);
    }

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == groupAction)
    {
        return groupNodes(selected.nodes);
    }
    if (triggered == deleteAction)
    {
        GtObjectList list;
        std::transform(selected.nodes.begin(), selected.nodes.end(),
                       std::back_inserter(list), [](NodeGraphicsObject* o) {
             return &o->node();
         });
        return (void)gtDataModel->deleteFromModel(list);
    }
}

// this is quite the convoluted mess...
constexpr size_t PRE_ALLOC = 32;

void
GraphScene::groupNodes(QVector<NodeGraphicsObject*> const& selectedNodeObjects)
{
    // get new node name
    GtInputDialog dialog(GtInputDialog::TextInput);
    dialog.setWindowTitle(tr("New Node Caption"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(tr("Enter a new caption for the grouped nodes"));
    dialog.setInitialTextValue(QStringLiteral("Graph"));
    if (!dialog.exec()) return;

    QString const& groupNodeName = dialog.textValue().trimmed();
    if (groupNodeName.isEmpty())
    {
        gtError() << tr("Failed to group nodes! (Invalid graph name)");
        return;
    }

#ifndef NDEBUG
    // timmer to track elapsed time
    QElapsedTimer timer;
    timer.start();
    auto finally = gt::finally([&timer](){
        gtTrace().verbose()
            << tr("Grouping nodes took %1 ms!").arg(timer.elapsed());
    });
#endif

    // separate connections into connections that are ingoing and outgoing to the group node
    // and internal connections that can be kept as is
    QVarLengthArray<ConnectionId, PRE_ALLOC> connectionsInternal, connectionsIn, connectionsOut;

    for (auto const* object : selectedNodeObjects)
    {
        NodeId nodeId = object->nodeId();
        assert (m_graph->findNode(object->nodeId()));

        // check connections
        auto& conModel = m_graph->connectionModel();
        for (ConnectionId conId : conModel.iterateConnections(nodeId))
        {
            auto const findNodeFunctor = [&conId](NodeGraphicsObject* o){
                return o->nodeId() == conId.inNodeId;
            };

            // if ingoing node is not part of nodes to group it is an outside connection
            if(std::find_if(selectedNodeObjects.begin(),
                            selectedNodeObjects.end(),
                            findNodeFunctor) == selectedNodeObjects.end())
            {
                connectionsOut.push_back(conId);
                continue;
            }

            // reverse conId so that outNodeId is now inNodeId
            conId.reverse();

            // if outgoing node is not part of nodes to group it is an outside connection
            if(std::find_if(selectedNodeObjects.begin(),
                            selectedNodeObjects.end(),
                            findNodeFunctor) == selectedNodeObjects.end())
            {
                // revert reverse
                conId.reverse();
                connectionsIn.push_back(conId);
                continue;
            }

            // revert reverse
            conId.reverse();
            // internal connections may already be contained
            if (!connectionsInternal.contains(conId))
            {
                connectionsInternal.push_back(conId);
            }
        }
    }

    // sort in and out going connections to avoid crossing connections
    auto const sortByScenePos = [this](ConnectionId a, ConnectionId b, bool reverse = false){
        auto oA = connectionObject(a);
        auto oB = connectionObject(b);
        assert(oA);
        assert(oB);
        return oA->endPoint(PortType::In).y() < oB->endPoint(PortType::In).y();
    };

    std::sort(connectionsIn.begin(),  connectionsIn.end(),
              std::bind(sortByScenePos, std::placeholders::_1, std::placeholders::_2, false));
    std::sort(connectionsOut.begin(), connectionsOut.end(),
              std::bind(sortByScenePos, std::placeholders::_1, std::placeholders::_2, true));

    // preprocess selected nodes
    QVector<Node*> selectedNodes;
    std::transform(selectedNodeObjects.begin(),
                   selectedNodeObjects.end(),
                   std::back_inserter(selectedNodes),
                   [](NodeGraphicsObject* o){ return &o->node(); });

    QPolygonF selectionPoly;
    std::transform(selectedNodes.begin(), selectedNodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    // update node positions
    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    auto offset = QPointF{boundingRect.width() * 0.5, boundingRect.height() * 0.5};

    // create group node
    auto tmpGraph = std::make_unique<Graph>();
    auto* groupNode = tmpGraph.get();

    groupNode->setCaption(groupNodeName);
    groupNode->setPos(center);

    // setup input/output provider
    groupNode->initInputOutputProviders();
    auto* inputProvider  = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << tr("Failed to group nodes! "
                    "(Invalid input or output provider)");
        return;
    }

    inputProvider->setPos(inputProvider->pos() - offset);
    outputProvider->setPos(outputProvider->pos() + offset);

    // find commections that share the same outgoing node and port
    auto const extractSharedConnections = [](auto& connections){
        QVarLengthArray<ConnectionId, PRE_ALLOC> shared;

        for (auto begin = connections.begin(); begin != connections.end(); ++begin)
        {
            auto iter = std::find_if(begin + 1, connections.end(),
                                     [conId = *begin](ConnectionId other){
                return conId.outNodeId == other.outNodeId &&
                       conId.outPort == other.outPort;
            });
            if (iter == connections.end()) continue;

            shared.push_back(*iter);
            connections.erase(iter);
        }

        return shared;
    };

    QVarLengthArray<ConnectionId, PRE_ALLOC> connectionsInShared  = extractSharedConnections(connectionsIn);
    QVarLengthArray<ConnectionId, PRE_ALLOC> connectionsOutShared = extractSharedConnections(connectionsOut);

    // helper function to extract and check typeIds
    auto const extractTypeIds = [this](auto const& connections){
        QVarLengthArray<QString, PRE_ALLOC> retval;

        for (ConnectionId conId : connections)
        {
            auto* node = m_graph->findNode(conId.inNodeId);
            assert(node);
            auto* port = node->port(conId.inPort);
            assert(port);

            if (!NodeDataFactory::instance().knownClass(port->typeId))
            {
                gtError() << tr("Failed to group nodes! "
                                "(Unkown node datatype '%1', id: %2, port: %3)")
                                 .arg(port->typeId,
                                      node->caption(),
                                      toString(*port));
                continue;
            }

            retval.push_back(port->typeId);
        }

        return retval;
    };

    // find datatype for input and output provider
    auto const& dtypeIn  = extractTypeIds(connectionsIn);
    auto const& dtypeOut = extractTypeIds(connectionsOut);

    if (dtypeIn.size()  != connectionsIn.size() ||
        dtypeOut.size() != connectionsOut.size()) return;

    // setup input and output ports
    for (QString const& typeId : dtypeIn ) inputProvider->addPort(typeId);
    for (QString const& typeId : dtypeOut) outputProvider->addPort(typeId);

    {

    // helper maps to track which index was already updated
    std::vector<bool> updatedIngoingConnectionsMap;
    updatedIngoingConnectionsMap.resize(connectionsIn.size());
    std::vector<bool> updatedOutgoingConnectionsMap;
    updatedOutgoingConnectionsMap.resize(connectionsOut.size());
    std::vector<bool> updatedInternalInConnectionsMap;
    updatedInternalInConnectionsMap.resize(connectionsInternal.size());
    std::vector<bool> updatedInternalOutConnectionsMap;
    updatedInternalOutConnectionsMap.resize(connectionsInternal.size());
    std::vector<bool> updatedSharedIngoingConnectionsMap;
    updatedSharedIngoingConnectionsMap.resize(connectionsInShared.size());
    std::vector<bool> updatedSharedOutgoingConnectionsMap;
    updatedSharedOutgoingConnectionsMap.resize(connectionsOutShared.size());

    // move selected nodes to subgraph and update their id
    for (auto* node : qAsConst(selectedNodes))
    {
        auto newNode = gt::unique_qobject_cast<Node>(
            std::unique_ptr<GtObject>(node->copy())
        );


        if (!newNode)
        {
            gtError() << tr("Failed to create group node! "
                            "(Nodes %1 could not be copied)").arg(node->id());
            return;
        }

        NodeId oldId = node->id();

        // reset nodeId
        newNode->setId(NodeId(0));
        newNode->setPos(newNode->pos() - center);

        // append new node
        auto* movedNode = groupNode->appendNode(std::move(newNode));
        if (!movedNode)
        {
            gtError() << tr("Failed to create group node! "
                            "(Node could not be moved)").arg(node->id());
            return;
        }

        // udpate connections if node id has changed
        NodeId newId = movedNode->id();
        if (newId == oldId) continue;

#ifndef NDEBUG
        gtTrace().verbose() << "Updating node id from" << oldId << "to" << newId << "...";
#endif

        // helper function to update the old node ids without overriding each entry multiple times
        auto const updateConnections = [](auto& connections,
                                          auto& map,
                                          size_t& index,
                                          NodeId oldId,
                                          NodeId newId,
                                          PortType type){
            index = 0;
            if (type == PortType::In)  for (ConnectionId& conId : connections)
            {
                if (!map[index] && conId.inNodeId == oldId)
                {
                    map[index] = true;
                    conId.inNodeId = newId;
                }
                index++;
            }
            index = 0;
            if (type == PortType::Out) for (ConnectionId& conId : connections)
            {
                if (!map[index] && conId.outNodeId == oldId)
                {
                    map[index] = true;
                    conId.outNodeId = newId;
                }
                index++;
            }
        };

        size_t index = 0;
        updateConnections(connectionsInShared,  updatedSharedIngoingConnectionsMap,  index, oldId, newId, PortType::In);
        updateConnections(connectionsIn,        updatedIngoingConnectionsMap,        index, oldId, newId, PortType::In);
        updateConnections(connectionsOutShared, updatedSharedOutgoingConnectionsMap, index, oldId, newId, PortType::Out);
        updateConnections(connectionsOut,       updatedOutgoingConnectionsMap,       index, oldId, newId, PortType::Out);
        // ingoing and outgoing connections must be tracked individually here
        updateConnections(connectionsInternal,  updatedInternalInConnectionsMap,     index, oldId, newId, PortType::In);
        updateConnections(connectionsInternal,  updatedInternalOutConnectionsMap,    index, oldId, newId, PortType::Out);
    }

    }

    // move group to graph
    auto appCmd = gtApp->makeCommand(m_graph, tr("Create group node '%1'").arg(groupNodeName));
    auto modifyCmd = m_graph->modify();

    Q_UNUSED(appCmd);
    Q_UNUSED(modifyCmd);

    if (!m_graph->appendNode(std::move(tmpGraph)))
    {
        gtError() << tr("Failed to group nodes! (Appending group node failed)");
        return;
    }

    // remove old nodes and connections. Connections must be deleted before
    // appending new connections
    qDeleteAll(selectedNodes);

    // helper function to create ingoing and outgoing connections
    auto const makeConnections = [this, groupNode](ConnectionId conId,
                                                   auto* provider,
                                                   PortIndex index,
                                                   PortType type,
                                                   bool addToMainGraph = true){
        if (type == PortType::Out) conId.reverse();

        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.inNodeId = groupNode->id();
        newCon.inPort   = groupNode->portId(type, index);

        // create connection in subgraph
        conId.outNodeId = provider->id();
        conId.outPort   = provider->portId(invert(type), index);

        assert(newCon.isValid());
        assert(conId .isValid());

        if (type == PortType::Out)
        {
            conId.reverse();
            newCon.reverse();
        }
        if (addToMainGraph)
        {
            m_graph->appendConnection(  std::make_unique<Connection>(newCon));
        }
        groupNode->appendConnection(std::make_unique<Connection>(conId ));
    };

    // create connections that share the same node and port
    auto const makeSharedConnetions = [makeConnections](auto& shared,
                                                        ConnectionId conId,
                                                        auto* provider,
                                                        PortIndex index,
                                                        PortType type){
        bool success = true;
        while (success)
        {
            auto iter = std::find_if(shared.begin(), shared.end(), [conId](ConnectionId other){
                return conId.outNodeId == other.outNodeId && conId.outPort == other.outPort;
            });
            if ((success = (iter != shared.end())))
            {
                makeConnections(*iter, provider, index, type, false);
                shared.erase(iter);
            }
        }
    };

    // make input connections
    PortIndex index{0};
    PortType type = PortType::In;
    for (ConnectionId conId : qAsConst(connectionsIn))
    {
        makeConnections(conId, inputProvider, index, type);

        makeSharedConnetions(connectionsInShared, conId, inputProvider, index, type);

        index++;
    }

    // make output connections
    index = PortIndex{0};
    type  = PortType::Out;
    for (ConnectionId conId : qAsConst(connectionsOut))
    {
        makeConnections(conId, outputProvider, index, type);

        makeSharedConnetions(connectionsOutShared, conId, inputProvider, index, type);

        index++;
    }

    // make internal connections
    for (ConnectionId const& conId : qAsConst(connectionsInternal))
    {
        groupNode->appendConnection(std::make_unique<Connection>(conId));
    }
}

void
GraphScene::onNodeAppended(Node* node)
{
    static NodeUI defaultUI;
    assert(node);

    NodeUI* ui = qobject_cast<NodeUI*>(gtApp->defaultObjectUI(node));
    if (!ui) ui = &defaultUI;

    auto entity = make_volatile<NodeGraphicsObject, DirectDeleter>(*m_sceneData, *node, *ui);
    // add to scene
    addItem(entity);

    // connect signals
    connect(entity, &NodeGraphicsObject::portContextMenuRequested,
            this, &GraphScene::onPortContextMenu);
    connect(entity, &NodeGraphicsObject::contextMenuRequested,
            this, &GraphScene::onNodeContextMenu);

    connect(entity, &NodeGraphicsObject::nodeShifted,
            this, &GraphScene::onNodeShifted, Qt::DirectConnection);
    connect(entity, &NodeGraphicsObject::nodeMoved,
            this, &GraphScene::onNodeMoved, Qt::DirectConnection);
    connect(entity, &NodeGraphicsObject::nodeDoubleClicked,
            this, &GraphScene::onNodeDoubleClicked, Qt::DirectConnection);
    connect(entity, &NodeGraphicsObject::nodeGeometryChanged,
            this, &GraphScene::moveConnections, Qt::DirectConnection);

    connect(entity, &NodeGraphicsObject::makeDraftConnection,
            this, &GraphScene::onMakeDraftConnection,
            Qt::DirectConnection);

    // append to map
    m_nodes.push_back({node->id(), std::move(entity)});
}

void
GraphScene::onNodeDeleted(NodeId nodeId)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
                             [nodeId](NodeEntry const& e){ return e.nodeId == nodeId; });
    if (iter != m_nodes.end())
    {
        m_nodes.erase(iter);
    }
}

void
GraphScene::onNodeShifted(NodeGraphicsObject* sender, QPointF diff)
{
    for (auto* o : Impl::findSelectedItems(*this, Impl::NodesOnly).nodes)
    {
        if (sender != o) o->moveBy(diff.x(), diff.y());
        moveConnections(o);
    }
}

void
GraphScene::onNodeMoved(NodeGraphicsObject* sender)
{
    for (auto* o : Impl::findSelectedItems(*this, Impl::NodesOnly).nodes)
    {
        o->commitPosition();
    }
}

void
GraphScene::onNodeDoubleClicked(NodeGraphicsObject* sender)
{
    assert(sender);

    Node& node = sender->node();

    Graph* graph = qobject_cast<Graph*>(&node);
    if (!graph)
    {
        return gt::gui::handleObjectDoubleClick(node);
    }

    emit graphNodeDoubleClicked(graph);
}

void
GraphScene::onConnectionAppended(Connection* con)
{
    assert(con);

    auto conId = con->connectionId();

    // access nodes and ports
    auto* inNodeEntity  = nodeObject(conId.inNodeId);
    assert(inNodeEntity);
    Node* inNode = &inNodeEntity->node();
    auto* inPort = inNode->port(conId.inPort);
    assert(inPort);

    auto* outNodeEntity = nodeObject(conId.outNodeId);
    assert(outNodeEntity);
    Node* outNode = &outNodeEntity->node();
    auto* outPort = outNode->port(conId.outPort);
    assert(outPort);

    auto entity = make_volatile<ConnectionGraphicsObject, DirectDeleter>(conId, outPort->typeId, inPort->typeId);
    entity->setConnectionShape(m_connectionShape);

    // add to scene
    addItem(entity);
    moveConnection(entity);

    // update type ids if port changes to make sure connections stay updated
    connect(inNode, &Node::portChanged, entity,
            [entity = entity.get(), inNode](PortId id){
        if (entity->connectionId().inPort != id) return;
        auto* port = inNode->port(id);
        assert(port);
        entity->setPortTypeId(PortType::In, port->typeId);
    });
    connect(outNode, &Node::portChanged, entity,
            [entity = entity.get(), outNode](PortId id){
        if (entity->connectionId().outPort != id) return;
        auto* port = outNode->port(id);
        assert(port);
        entity->setPortTypeId(PortType::Out, port->typeId);
    });

    // append to map
    m_connections.push_back({conId, std::move(entity)});

    // update in and out node
    inNodeEntity->update();
    outNodeEntity->update();
}

void
GraphScene::onConnectionDeleted(ConnectionId conId)
{
    auto iter = std::find_if(m_connections.begin(),
                             m_connections.end(),
                             [conId](ConnectionEntry const& e){
        return e.conId == conId;
    });

    if (iter != m_connections.end()) m_connections.erase(iter);

    // update in and out node
    auto* inNode  = nodeObject(conId.inNodeId);
    assert(inNode);
    inNode->update();

    auto* outNode = nodeObject(conId.outNodeId);
    assert(outNode);
    outNode->update();
}

void
GraphScene::moveConnection(ConnectionGraphicsObject* object,
                           NodeGraphicsObject* node)
{
    assert(object);

    bool isInNode  = !node || node->nodeId() == object->connectionId().inNodeId;
    bool isOutNode = !node || !isInNode;

    if (isInNode)
    {
        moveConnectionPoint(*object, PortType::In);
    }
    if (isOutNode)
    {
        moveConnectionPoint(*object, PortType::Out);
    }
}

void
GraphScene::moveConnectionPoint(ConnectionGraphicsObject& object, PortType type)
{
    auto const& conId = object.connectionId();

    NodeId nodeId = conId.node(type);
    assert(nodeId != invalid<NodeId>());

    NodeGraphicsObject* nObject = nodeObject(nodeId);
    if (!nObject) return;

    Impl::moveConnectionPoint(object, *nObject, type, conId.port(type));
}

void
GraphScene::moveConnections(NodeGraphicsObject* object)
{
    assert(object);

    auto& conModel = m_graph->connectionModel();
    for (auto const& conId : conModel.iterateConnections(object->nodeId()))
    {
        if (ConnectionGraphicsObject* con = connectionObject(conId))
        {
            moveConnection(con, object);
        }
    }
}

void
GraphScene::onMakeDraftConnection(NodeGraphicsObject* object,
                                  PortType type,
                                  PortId portId)
{
    assert(object);

    if (type == PortType::In)
    {
        // disconnect existing ingoing connection and make it a draft connection
        auto& conModel = m_graph->connectionModel();
        auto connections = conModel.iterateConnections(object->nodeId(), portId);
        if (!connections.empty())
        {
            assert(connections.size() == 1);
            return Impl::makeDraftConnection(*this, *object, *connections.begin());
        }
    }

    // create new connection
    Impl::instantiateDraftConnection(*this, *object, type, portId);
}
