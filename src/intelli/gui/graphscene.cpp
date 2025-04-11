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
#include <intelli/connection.h>
#include <intelli/nodefactory.h>
#include <intelli/nodedatafactory.h>
#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>
#include <intelli/gui/nodeui.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/connectionobject.h>
#include <intelli/gui/graphics/popupitem.h>
#include <intelli/private/utils.h>
#include <intelli/private/gui_utils.h>

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
 * @brief Helper method that checks if a node is user deletable
 * @param o Object
 * @return Whether the object is user deletable
 */
static bool
isNotDeletable(NodeGraphicsObject* o)
{
    return !(o->node().objectFlags() & GtObject::ObjectFlag::UserDeletable);
};

// TODO
static bool
isCollapsed(NodeGraphicsObject* o)
{
    return o->isCollpased();
};

// TODO
template <typename Func>
static auto
negate(Func func)
{
    return [f = std::move(func)](NodeGraphicsObject* o) -> bool {
        return !f(o);
    };
};

/**
 * @brief Returns a functor that searches for the targeted `nodeId`
 * @param nodeId
 */
static auto
findObjectById(NodeId nodeId)
{
    return [nodeId](NodeEntry const& e){
        return e.nodeId == nodeId;
    };
}

/**
 * @brief Returns a functor that searches for the targeted `conId`
 * @param nodeId
 */
static auto
findObjectById(ConnectionId conId)
{
    return [conId](ConnectionEntry const& e){
        return e.conId == conId;
    };
}

/**
 * @brief Returns the currently active view
 * @param scene Graph Scene
 * @return Active view (may be null)
 */
static QGraphicsView const*
activeView(GraphScene const* scene)
{
    auto const& views = scene->views();
    auto iter = std::find_if(views.begin(), views.end(), [](QGraphicsView const* view){
        return view->hasFocus();
    });
    return (iter != views.end()) ? *iter : nullptr;
}

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

    auto entity = make_unique_qptr<ConnectionGraphicsObject>(draftConId, *outType, *inType);
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
    auto oldConnection = scene.graph().findConnection(conId);
    assert(oldConnection);

    bool success = gtDataModel->deleteFromModel(oldConnection);
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

/**
 * @brief Copies the current selection (only eligible nodes and connections)
 * to the `dummy` graph
 * @param scene Scene
 * @param dummy Dummy graph to copy selection to
 * @return success
 */
static bool
copySelectionTo(GraphScene& scene, Graph& dummy)
{
    auto selected = Impl::findSelectedItems(scene);
    if (selected.nodes.empty()) return false;

    // remove unqiue nodes
    auto const isUnique = [](NodeGraphicsObject const* o){
        return o->node().nodeFlags() & NodeFlag::Unique;
    };

    selected.nodes.erase(
        std::remove_if(selected.nodes.begin(),
                       selected.nodes.end(),
                       isUnique),
        selected.nodes.end());

    // remove not deletable nodes
    selected.nodes.erase(
        std::remove_if(selected.nodes.begin(),
                       selected.nodes.end(),
                       isNotDeletable),
        selected.nodes.end());

    // at least one node should be selected
    if (selected.nodes.empty()) return false;

    // only duplicate internal connections
    auto const isExternalConnection = [&selected](ConnectionGraphicsObject const* o){
        auto con = o->connectionId();
        return !containsNodeId(con.inNodeId, selected.nodes) ||
               !containsNodeId(con.outNodeId, selected.nodes);
    };

    selected.connections.erase(
        std::remove_if(selected.connections.begin(),
                       selected.connections.end(),
                       isExternalConnection),
        selected.connections.end());

    // append nodes and connections to dummy graph
    bool success = true;
    for (auto* o : qAsConst(selected.nodes))
    {
        success &= !!dummy.appendNode(makeCopy(o->node()));
    }
    for (auto* o : qAsConst(selected.connections))
    {
        success &= !!dummy.appendConnection(std::make_unique<Connection>(o->connectionId()));
    }
    return success;
}

/**
 * @brief Pastes the objects in the `dummy` graph to the current graph
 * and updates the selection
 * @param scene Scene
 * @param dummy Dummy graph to paste from
 * @return success
 */
static bool
pasteFrom(GraphScene& scene, Graph& dummy)
{
    // regenerate uuids
    dummy.newUuid(true);
    dummy.resetGlobalConnectionModel();

    auto& conModel = dummy.connectionModel();
    if (conModel.empty()) return false;

    auto nodes = dummy.nodes();
    bool hasConnections = conModel.hasConnections();

    // shift node positions
    constexpr QPointF offset{50, 50};
    for (auto& entry : conModel)
    {
        Node* node = entry.node;
        node->setPos(node->pos() + offset);
    }

    auto cmd = gtApp->makeCommand(&scene.graph(), tr("Paste objects"));
    Q_UNUSED(cmd);

    // append objects
    bool success = dummy.moveNodesAndConnections(nodes, scene.graph());
    if (!success)
    {
        gtWarning() << tr("Pasting selection failed!");

        // restore previous state
        cmd.finalize();
        gtApp->undoStack()->undo();
        return false;
    }

    // find affected graphics objects and set selection
    scene.clearSelection();

    auto nodeObjects = Impl::findItems<NodeGraphicsObject*>(scene);
    for (auto* nodeObj : qAsConst(nodeObjects))
    {
        if (containsNodeId(nodeObj->nodeId(), nodes))
        {
            nodeObj->setSelected(true);
        }
    }

    if (!hasConnections) return true;

    auto connectionObjects = Impl::findItems<ConnectionGraphicsObject*>(scene);
    for (auto* conObj : qAsConst(connectionObjects))
    {
        auto conId = conObj->connectionId();
        if (containsNodeId(conId.inNodeId, nodes) && containsNodeId(conId.outNodeId, nodes))
        {
            conObj->setSelected(true);
        }
    }
    return true;
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

    connect(m_graph, &Graph::graphAboutToBeDeleted, this, &QObject::deleteLater);
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
                             Impl::findObjectById(nodeId));
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
    auto iter = std::find_if(m_connections.begin(),
                             m_connections.end(),
                             Impl::findObjectById(conId));
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

    for (QString const& cat : qAsConst(cats))
    {
        if (cat.isEmpty()) continue;

        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }

    auto nodes = factory.registeredNodes();

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
    auto selected = Impl::findSelectedItems(*this);
    if (selected.empty()) return;

    // remove nodes that are not deletable
    auto beginErase = std::remove_if(selected.nodes.begin(),
                                     selected.nodes.end(),
                                     Impl::isNotDeletable);

    // not all nodes are default deletable
    int count = std::distance(beginErase, selected.nodes.end());
    if (count == selected.nodes.size() && !selected.nodes.empty())
    {
        if (count == 1)
        {
            // attempt to find custom delete action
            Node& node = selected.nodes.front()->node();

            QKeySequence shortcut = gtApp->getShortCutSequence("delete");

            GtObjectUIAction action = gui_utils::findUIActionByShortCut(node, shortcut);
            if (action.method())
            {
                return action.method()(nullptr, &node);
            }
            // node not deletable
        }

        // create popup to notify that not all nodes are deletable
        if (QGraphicsView const* view = Impl::activeView(this))
        {
            auto* popup = PopupItem::addPopupItem(
                *this,
                tr("Selected %1 not deletable!")
                    .arg(selected.nodes.size() == 1 ? tr("node is") :
                                                      tr("nodes are")),
                std::chrono::seconds{1}
            );
            // apply cursor position
            QPointF pos = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
            pos.rx() -= popup->boundingRect().width() * 0.5;
            pos.ry() -= popup->boundingRect().height() * 1.5;
            popup->setPos(pos);
        }
    }

    // remove not deletable nodes
    selected.nodes.erase(beginErase, selected.nodes.end());

    if (selected.empty()) return;

    // default delete selected nodes
    GtObjectList objects;
    std::transform(selected.connections.begin(),
                   selected.connections.end(),
                   std::back_inserter(objects),
                   [this](ConnectionGraphicsObject* o){
        return m_graph->findConnection(o->connectionId());
    });
    std::transform(selected.nodes.begin(),
                   selected.nodes.end(),
                   std::back_inserter(objects),
                   [](NodeGraphicsObject* o){
        return &o->node();
    });

    auto modifyCmd = m_graph->modify();
    Q_UNUSED(modifyCmd);

    gtDataModel->deleteFromModel(objects);
}

void
GraphScene::duplicateSelectedObjects()
{
    // bypass clipboard
    Graph dummy;
    if (Impl::copySelectionTo(*this, dummy))
    {
        Impl::pasteFrom(*this, dummy);
    }
}

bool
GraphScene::copySelectedObjects()
{
    Graph dummy;
    if (!Impl::copySelectionTo(*this, dummy)) return false;

    QByteArray mementoData = dummy.toMemento().toByteArray();
    QApplication::clipboard()->setText(std::move(mementoData));
    return true;
}

void
GraphScene::pasteObjects()
{
    // read from clipboard
    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    // restore objects
    GtObjectMemento mem(text.toUtf8());
    if (mem.isNull()) return;

    auto dummy = gt::unique_qobject_cast<Graph>(mem.toObject(*gtObjectFactory));
    if (!dummy) return;

    Impl::pasteFrom(*this, *dummy);
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

    // add custom actions
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

    bool allDeletable = std::none_of(selected.nodes.begin(),
                                     selected.nodes.end(),
                                     Impl::isNotDeletable);

    bool someCollapsed = std::any_of(selected.nodes.begin(),
                                     selected.nodes.end(),
                                     Impl::isCollapsed);

    bool someUncollapsed = std::any_of(selected.nodes.begin(),
                                       selected.nodes.end(),
                                       Impl::negate(Impl::isCollapsed));

    Node* selectedNode = &selected.nodes.at(0)->node();
    assert(selectedNode);
    Graph* selectedGraphNode = NodeUI::toGraph(selectedNode);

    // create menu
    QMenu menu;

    QAction* ungroupAction = menu.addAction(tr("Expand Subgraph"));
    ungroupAction->setIcon(gt::gui::icon::stretch());
    ungroupAction->setEnabled(allDeletable);
    ungroupAction->setVisible(selectedGraphNode && selected.nodes.size() == 1);

    QAction* groupAction = menu.addAction(tr("Group selected Nodes"));
    groupAction->setIcon(gt::gui::icon::select());
    groupAction->setEnabled(allDeletable);

    QAction* collapseAction = menu.addAction(tr("Collapse selected Nodes"));
    collapseAction->setIcon(gt::gui::icon::triangleUp());
    collapseAction->setVisible(someUncollapsed);

    QAction* uncollapseAction = menu.addAction(tr("Uncollapse selected Nodes"));
    uncollapseAction->setIcon(gt::gui::icon::triangleDown());
    uncollapseAction->setVisible(someCollapsed);

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
    if (triggered == ungroupAction)
    {
        return expandGroupNode(selectedGraphNode);
    }
    if (triggered == collapseAction ||
        triggered == uncollapseAction)
    {
        return collapseNodes(selected.nodes, triggered == collapseAction);
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

    // preprocess nodes
    QVector<Node*> selectedNodes;
    std::transform(selectedNodeObjects.begin(),
                   selectedNodeObjects.end(),
                   std::back_inserter(selectedNodes),
                   [](NodeGraphicsObject* o){ return &o->node(); });

    QVector<ConnectionUuid> connectionsIn, connectionsOut;

    auto& conModel = m_graph->connectionModel();

    // separate connections into ingoing and outgoing of the group node
    for (Node const* node : qAsConst(selectedNodes))
    {
        for (ConnectionId conId : conModel.iterateConnections(node->id()))
        {
            if (!containsNodeId(conId.inNodeId, selectedNodes))
            {
                connectionsOut.push_back(m_graph->connectionUuid(conId));
            }
            if (!containsNodeId(conId.outNodeId, selectedNodes))
            {
                connectionsIn.push_back(m_graph->connectionUuid(conId));
            }
        }
    }

    // sort in and out going connections to avoid crossing connections
    auto const sortByEndPoint = [this](ConnectionUuid const& a,
                                       ConnectionUuid const& b){
        auto oA = connectionObject(m_graph->connectionId(a));
        auto oB = connectionObject(m_graph->connectionId(b));
        assert(oA);
        assert(oB);
        return oA->endPoint(PortType::In).y() <
               oB->endPoint(PortType::In).y();
    };

    std::sort(connectionsIn.begin(),  connectionsIn.end(),  sortByEndPoint);
    std::sort(connectionsOut.begin(), connectionsOut.end(), sortByEndPoint);

    // create undo command
    auto appCmd = gtApp->makeCommand(m_graph, tr("Create group node '%1'").arg(groupNodeName));
    auto modifyCmd = m_graph->modify();
    Q_UNUSED(modifyCmd);

    auto restoreCmd = gt::finally([&appCmd](){
        appCmd.finalize();
        gtApp->undoStack()->undo();
    });

    // create group node
    auto targetGraphPtr = std::make_unique<Graph>();
    targetGraphPtr->setCaption(groupNodeName);

    // setup input/output provider
    targetGraphPtr->initInputOutputProviders();
    auto* inputProvider  = targetGraphPtr->inputProvider();
    auto* outputProvider = targetGraphPtr->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << tr("Failed to group nodes! "
                        "(Invalid input or output provider)");
        return;
    }

    // update node positions
    QPolygonF selectionPoly;
    std::transform(selectedNodes.begin(), selectedNodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    auto offset = QPointF{boundingRect.width() * 0.5,
                          boundingRect.height() * 0.5};

    targetGraphPtr->setPos(center);
    inputProvider->setPos(inputProvider->pos() + center - 2 * offset);
    outputProvider->setPos(outputProvider->pos() + center);

    for (Node* node : qAsConst(selectedNodes))
    {
        node->setPos(node->pos() - offset);
    }

    // find connections that share the same outgoing node and port
    auto const extractSharedConnections = [](auto& connections){
        QVector<ConnectionUuid> shared;

        for (auto begin = connections.begin(); begin != connections.end(); ++begin)
        {
            auto iter = std::find_if(std::next(begin, 1), connections.end(),
                                     [conId = *begin](ConnectionUuid const& other){
                return conId.outNodeId == other.outNodeId &&
                       conId.outPort == other.outPort;
            });
            if (iter == connections.end()) continue;

            shared.push_back(*iter);
            connections.erase(iter);
        }

        return shared;
    };

    auto connectionsInShared  = extractSharedConnections(connectionsIn);
    auto connectionsOutShared = extractSharedConnections(connectionsOut);

    // helper function to extract and check typeIds
    auto const extractTypeIds = [this](auto const& connections){
        QVector<QString> retval;

        for (ConnectionUuid const& conId : connections)
        {
            auto* node = m_graph->findNodeByUuid(conId.inNodeId);
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

    Graph* targetGraph = m_graph->appendNode(std::move(targetGraphPtr));
    // append node first
    if (!targetGraph)
    {
        gtError() << tr("Failed to group nodes! (Appending group node failed)");
        return;
    }

    // move nodes and internal connections
    if (!m_graph->moveNodesAndConnections(selectedNodes, *targetGraph))
    {
        gtError() << tr("Failed to group nodes! (Moving nodes failed)");
        return;
    }

    // helper function to create ingoing and outgoing connections
    auto const makeConnections = [this, targetGraph](ConnectionUuid conUuid,
                                                     auto* provider,
                                                     PortIndex index,
                                                     PortType type,
                                                     bool addToMainGraph = true,
                                                     bool addToTargetGrtaph = true) {
        if (type == PortType::Out) conUuid.reverse();

        // create connection in parent graph
        if (addToMainGraph)
        {
            ConnectionUuid newCon = conUuid;
            newCon.inNodeId = targetGraph->uuid();
            newCon.inPort   = targetGraph->portId(type, index);
            assert(newCon.isValid());

            if (type == PortType::Out) newCon.reverse();

            m_graph->appendConnection(m_graph->connectionId(newCon));
        }
        // create connection in subgraph
        if (addToTargetGrtaph)
        {
            conUuid.outNodeId = provider->uuid();
            conUuid.outPort   = provider->portId(invert(type), index);
            assert(conUuid .isValid());

            if (type == PortType::Out) conUuid.reverse();

            targetGraph->appendConnection(targetGraph->connectionId(conUuid));
        }
    };

    // create connections that share the same node and port
    auto const makeSharedConnetions = [makeConnections](auto& shared,
                                                        ConnectionUuid conUuid,
                                                        auto* provider,
                                                        PortIndex index,
                                                        PortType type){
        bool success = true;
        while (success)
        {
            auto iter = std::find_if(shared.begin(), shared.end(),
                                     [conUuid](ConnectionUuid const& other){
                return conUuid.outNodeId == other.outNodeId &&
                       conUuid.outPort == other.outPort;
            });
            if ((success = (iter != shared.end())))
            {
                bool installInParent = type == PortType::Out;
                makeConnections(*iter, provider, index, type, installInParent, !installInParent);
                shared.erase(iter);
            }
        }
    };

    // make input connections
    PortIndex index{0};
    PortType type = PortType::In;
    for (ConnectionUuid const& conId : qAsConst(connectionsIn))
    {
        makeConnections(conId, inputProvider, index, type);

        makeSharedConnetions(connectionsInShared, conId, inputProvider, index, type);

        index++;
    }

    // make output connections
    index = PortIndex{0};
    type  = PortType::Out;
    for (ConnectionUuid const& conId : qAsConst(connectionsOut))
    {
        makeConnections(conId, outputProvider, index, type);

        makeSharedConnetions(connectionsOutShared, conId, outputProvider, index, type);

        index++;
    }

    restoreCmd.clear();
}

void
GraphScene::expandGroupNode(Graph* groupNode)
{
    assert(groupNode);

    // create undo command
    auto appCmd = gtApp->makeCommand(m_graph, tr("Expand group node '%1'").arg(groupNode->caption()));
    auto modifyGroupCmd = groupNode->modify();
    auto modifyCmd = m_graph->modify();
    Q_UNUSED(modifyCmd);

    auto restoreCmd = gt::finally([&appCmd](){
        appCmd.finalize();
        gtApp->undoStack()->undo();
    });

    auto const& conModel = m_graph->connectionModel();

    auto* inputProvider  = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();
    assert(inputProvider);
    assert(outputProvider);

    // gather input and output connections
    QVector<ConnectionUuid> expandedInputConnections, expandedOutputConnections;

    // extra scope since group node will be deleted eventually -> avoid dangling
    // references
    {
        // "flatten" connections between parent graph and subgraph
        auto convertConnection =
            [this, &conModel, groupNode](ConnectionId conId,
                                         auto& convertedConnections,
                                         PortType type){
            ConnectionUuid conUuid = groupNode->connectionUuid(conId);

            bool const isInput = type == PortType::In;
            if (isInput) conUuid.reverse();

            auto connections = conModel.iterate(groupNode->id(), conUuid.outPort);
            for (auto const& connection : connections)
            {
                NodeId targetNodeId = connection.node;
                Node* targetNode = m_graph->findNode(targetNodeId);
                assert(targetNode);
                conUuid.outNodeId = targetNode->uuid();
                conUuid.outPort   = connection.port;

                convertedConnections.push_back(isInput ? conUuid.reversed() : conUuid);
            }
        };

        auto const& groupConModel = groupNode->connectionModel();

        PortType type = PortType::Out;
        auto inputCons  = groupConModel.iterateConnections(inputProvider->id(), type);
        for (ConnectionId conId : inputCons)
        {
            convertConnection(conId, expandedInputConnections, type);
        }

        type = PortType::In;
        auto outputCons = groupConModel.iterateConnections(outputProvider->id(), type);
        for (ConnectionId conId : outputCons)
        {
            convertConnection(conId, expandedInputConnections, type);
        }
    }

    // delete provider nodes
    if (!groupNode->deleteNode(groupNode->inputNode()->id()) ||
        !groupNode->deleteNode(groupNode->outputNode()->id()))
    {
        gtError() << tr("Expanding group node '%1' failed! "
                        "(Failed to remove provider nodes)")
                         .arg(relativeNodePath(*groupNode));
        return;
    }

    inputProvider = nullptr;
    outputProvider = nullptr;
    auto nodes = groupNode->nodes();

    // update node positions
    QPolygonF selectionPoly;
    std::transform(nodes.begin(), nodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    for (Node* node : qAsConst(nodes))
    {
        auto offset = (node->pos() - center);
        node->setPos(groupNode->pos() + offset);
    }

    // move internal connections
    if (!groupNode->moveNodesAndConnections(nodes, *m_graph))
    {
        gtError() << tr("Expanding group node '%1' failed! "
                        "(Failed to move internal nodes)")
                         .arg(relativeNodePath(*groupNode));
        return;
    }

    // delete group node
    modifyGroupCmd.finalize();
    delete groupNode;
    groupNode = nullptr;

    // install connections to moved nodes
    for (auto* connections : {&expandedInputConnections, &expandedOutputConnections})
    {
        for (ConnectionUuid const& conUuid : qAsConst(*connections))
        {
            m_graph->appendConnection(m_graph->connectionId(conUuid));
        }
    }

    restoreCmd.clear();
}

void
GraphScene::collapseNodes(QVector<NodeGraphicsObject*> const& selectedNodeObjects,
                          bool doCollapse)
{
    for (NodeGraphicsObject* o : selectedNodeObjects)
    {
        o->collapse(doCollapse);
    }
}

void
GraphScene::onNodeAppended(Node* node)
{
    static NodeUI defaultUI;
    assert(node);

    NodeUI* ui = qobject_cast<NodeUI*>(gtApp->defaultObjectUI(node));
    if (!ui) ui = &defaultUI;

    auto entity = make_unique_qptr<NodeGraphicsObject, DirectDeleter>(*m_sceneData, *node, *ui);
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
    connect(entity, &NodeGraphicsObject::nodePositionChanged,
            this, &GraphScene::onNodePositionChanged, Qt::DirectConnection);
    connect(entity, &NodeGraphicsObject::nodeDoubleClicked,
            this, &GraphScene::onNodeDoubleClicked, Qt::DirectConnection);
    connect(entity, &NodeGraphicsObject::nodeGeometryChanged,
            this, &GraphScene::moveConnections, Qt::DirectConnection);

    connect(entity, &NodeGraphicsObject::makeDraftConnection,
            this, &GraphScene::onMakeDraftConnection,
            Qt::DirectConnection);

    auto* entityPtr = entity.get();

    // append to map
    m_nodes.push_back({node->id(), std::move(entity)});

    emit nodeAppended(entityPtr);
}

void
GraphScene::onNodeDeleted(NodeId nodeId)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
                             Impl::findObjectById(nodeId));
    if (iter != m_nodes.end())
    {
        m_nodes.erase(iter);
    }
}

void
GraphScene::onNodeShifted(NodeGraphicsObject* sender, QPointF diff)
{
    auto const selection = Impl::findSelectedItems(*this, Impl::NodesOnly);

    if (!m_nodeMoveCmd.isValid())
    {
        QString const txt = selection.nodes.size() > 1 ?
                                tr("Nodes moved") :
                                tr("Node '%1' moved")
                                    .arg(relativeNodePath(selection.nodes.at(0)->node()));
        m_nodeMoveCmd = gtApp->startCommand(m_graph, txt);
    }

    for (auto* o : selection.nodes)
    {
        if (sender != o) o->moveBy(diff.x(), diff.y());
        moveConnections(o);
    }
}

void
GraphScene::onNodeMoved(NodeGraphicsObject* sender)
{
    // nodes have not been moved
    if (!m_nodeMoveCmd.isValid()) return;

    auto const selection = Impl::findSelectedItems(*this, Impl::NodesOnly);
    // commit position to data model
    for (auto* o : selection.nodes)
    {
        o->commitPosition();
    }

    // finish command
    gtApp->endCommand(m_nodeMoveCmd);
    m_nodeMoveCmd = {};
}

void
GraphScene::onNodePositionChanged(NodeGraphicsObject* sender)
{
    moveConnections(sender);
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

    auto entity = make_unique_qptr<ConnectionGraphicsObject, DirectDeleter>(conId, outPort->typeId, inPort->typeId);
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
                             Impl::findObjectById(conId));
    if (iter == m_connections.end())
    {
        gtError() << utils::logId(this)
                  << tr("Failed to remove connection:") << conId;
        return;
    }

    m_connections.erase(iter);

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
