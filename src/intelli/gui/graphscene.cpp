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
#include <intelli/graphutilities.h>
#include <intelli/connection.h>
#include <intelli/nodefactory.h>
#include <intelli/nodedatafactory.h>
#include <intelli/utilities.h>
#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>
#include <intelli/gui/commentgroup.h>
#include <intelli/gui/commentdata.h>
#include <intelli/gui/guidata.h>
#include <intelli/gui/nodeui.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/commentobject.h>
#include <intelli/gui/graphics/connectionobject.h>
#include <intelli/gui/graphics/nodeobject.h>
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
#include <QPushButton>
#include <QTreeWidget>
#include <QHeaderView>
#include <QVarLengthArray>

using namespace intelli;

/// creates a copy of the object and returns a unique_ptr
template <typename T>
inline auto makeCopy(T& obj)
{
    std::unique_ptr<GtObject> tmp{obj.copy()};
    return gt::unique_qobject_cast<std::remove_const_t<T>>(std::move(tmp));
}

struct GraphScene::Impl
{

/**
 * @brief Helper method that checks if a node is default deletable
 * @param o Object
 * @return Whether the object is user deletable
 */
static bool
isDefaultDeletable(InteractableGraphicsObject* o)
{
    return (o->deletableFlag() == GraphicsObject::DefaultDeletable);
}

/**
 * @brief Helper method that yields whether a node is collapsed
 * @param o Object
 * @return Whether the node is collapsed
 */
static bool
isCollapsed(InteractableGraphicsObject* o)
{
    return o->isCollapsed();
}

/**
 * @brief Negates the result of a functor.
 * @param func Functor, must yield a boolean compatible value and accept the
 * pointer to a node graphics object as an argument.
 */
template <typename Func>
static auto
negate(Func func)
{
    return [f = std::move(func)](auto* o) -> bool {
        return !f(o);
    };
}

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
 * @brief Returns a functor that searches for the targeted `comment`
 * @param nodeId
 */
static auto
findObjectById(CommentData* comment)
{
    return [uuid = comment->uuid()](CommentEntry const& e){
        return e.uuid == uuid;
    };
}

/**
 * @brief Find all items of type T in `list`
 * @param scene Scene object (this)
 * @return All graphic items of type T in `list`
 */
template<typename T, typename List>
static QVector<T>
findItems(GraphScene& scene, List const& list)
{
    QVector<T> items;
    for (auto* item : list)
    {
        if (auto* obj = graphics_cast<T>(item))
        {
            items << obj;
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
    return findItems<T>(scene, scene.items());
}

/**
 * @brief Find all selected items of type T
 * @param scene Scene object (this)
 * @return All selected graphic items of type T
 */
template<typename T>
static QVector<T>
findSelectedItems(GraphScene& scene)
{
    return findItems<T>(scene, scene.selectedItems());
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

    // find nodes that can potentially recieve a connection
    // -> all nodes that we do not depend on/that do not depend on us
    QSet<NodeId> dependencies;

    auto const& conModel = scene.graph().connectionModel();
    auto allNodes = conModel.iterateNodeIds();

    auto accumulate = [&conModel](QSet<NodeId>& storage, auto const& range, PortType type, auto const& lambda) -> void {
        for (NodeId nodeId : range)
        {
            if (storage.contains(nodeId)) continue;
            storage.insert(nodeId);
            lambda(storage, conModel.iterateNodes(nodeId, type), type, lambda);
        }
    };

    accumulate(dependencies, conModel.iterateNodes(sourceNodeId, PortType::In ), PortType::In , accumulate);
    accumulate(dependencies, conModel.iterateNodes(sourceNodeId, PortType::Out), PortType::Out, accumulate);
    dependencies.insert(sourceNodeId);

    QVector<NodeId> targets;
    std::set_difference(allNodes.begin(), allNodes.end(),
                        dependencies.begin(), dependencies.end(),
                        std::back_inserter(targets));

    // "unhighlight" all dependencies and dependent nodes
    for (NodeId nodeId : qAsConst(dependencies))
    {
        NodeGraphicsObject* target = scene.nodeObject(nodeId);
        assert(target);
        target->highlights().setAsIncompatible();
    }
    // highlight all potential target nodes
    for (NodeId nodeId : qAsConst(targets))
    {
        NodeGraphicsObject* target = scene.nodeObject(nodeId);
        assert(target);
        target->highlights().setCompatiblePorts(sourcePort.typeId, invert(type));
    }

    // override source port
    NodeGraphicsObject* source = scene.nodeObject(sourceNodeId);
    assert(source);
    source->highlights().setAsIncompatible();
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
    assert(sourcePortId.isValid());

    NodeId sourceNodeId = sourceObject.nodeId();

    auto& sourceNode = sourceObject.node();

    auto* sourcePort = sourceNode.port(sourcePortId);
    assert(sourcePort);

    // dummy connection (respective end point is not connected)
    ConnectionId draftConId{
        sourceNodeId,
        sourcePortId,
        invalid<NodeId>(),
        invalid<PortId>()
    };

    if (sourceType == PortType::In) draftConId.reverse();

    assert(draftConId.isDraft());
    assert(draftConId.draftType() == sourceType);

    auto entity = convert_to_unique_qptr<DirectDeleter>(
        ConnectionGraphicsObject::makeDraftConnection(scene, draftConId, sourceObject)
    );
    entity->setConnectionShape(scene.m_connectionShape);

    // move respective end point to start point and grab mouse
    entity->setEndPoint(invert(sourceType), entity->endPoint(sourceType));

    connect(&sourceObject, &QObject::destroyed, entity, [c = entity.get()](){ delete c; });

    highlightCompatibleNodes(scene, sourceNode, *sourcePort);

    connect(entity, &ConnectionGraphicsObject::finalizeDraftConnnection,
            &scene, &GraphScene::onFinalizeDraftConnection);

    return entity.release();
}

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
 * @brief Collapsed/expands the selected nodes
 * @param selected Objects that should be collapsed/expanded
 * @param doCollapse Whether the nodes should be collapsed or expanded
 */
static void
collapseObjects(GraphScene& scene,
                QVector<InteractableGraphicsObject*> const& selected,
                bool doCollapse)
{
    if (selected.empty()) return;

    auto const& selectedNodes = findItems<NodeGraphicsObject*>(scene, selected);

    QString caption = QChar{'('} +
                      (selectedNodes.size() > 1 ?
                           relativeNodePath(selectedNodes.front()->node()) +
                               (selectedNodes.size() > 1 ? ", ...":"") :
                           QString{"..."}) +
                      QChar{')'};

    auto change = gtApp->makeCommand(
        &scene.graph(),
        tr("Object%1 %2collapsed %3")
            .arg(selected.size() > 1 ? "s":"", doCollapse? "":"un", caption)
    );
    Q_UNUSED(change);

    for (InteractableGraphicsObject* o : selected)
    {
        o->collapse(doCollapse);
    }
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
    auto selectedObjects= Impl::findSelectedItems<InteractableGraphicsObject*>(scene);
    if (selectedObjects.empty()) return false;

    QVector<ObjectUuid> selection;

    std::transform(selectedObjects.begin(),
                   selectedObjects.end(),
                   std::back_inserter(selection),
                   [](InteractableGraphicsObject* object){
        assert(object);
        return object->objectUuid();
    });

    return utils::copyObjectsToGraph(scene.graph(), selection, dummy);
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
    scene.clearSelection();

    // shift all new objects and make them selected
    auto scope = utils::connectScoped(&scene, &GraphScene::objectAdded,
                                      &scene, [](InteractableGraphicsObject* object){
        object->shiftBy(50, 50);
        object->commitPosition();
        object->setSelected(true);
    });
    Q_UNUSED(scope);
    auto cmd = gtApp->makeCommand(&scene.graph(), tr("Paste objects"));
    Q_UNUSED(cmd);

    return utils::moveObjectsToGraph(dummy, scene.graph());
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

    // comments
    CommentGroup* commentGroup = GuiData::accessCommentGroup(*m_graph);
    if (commentGroup)
    {
        auto const& comments = commentGroup->comments();
        for (auto* comment : comments)
        {
            onCommentAppended(comment);
        }

        connect(commentGroup, &CommentGroup::commentAppended,
                this, &GraphScene::onCommentAppended, Qt::DirectConnection);
        connect(commentGroup, &CommentGroup::commentAboutToBeDeleted,
                this, &GraphScene::onCommentDeleted, Qt::DirectConnection);
    }

    connect(m_graph, &Graph::nodeAppended,
            this, &GraphScene::onNodeAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::childNodeAboutToBeDeleted,
            this, &GraphScene::onNodeDeleted, Qt::DirectConnection);

    connect(m_graph, &Graph::connectionAppended,
            this, &GraphScene::onConnectionAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::connectionDeleted,
            this, &GraphScene::onConnectionDeleted, Qt::DirectConnection);

    // graphics objects must be deleted to not access deleted nodes
    connect(m_graph, &Graph::graphAboutToBeDeleted,
            this, [this](){
        m_comments.clear();
        m_connections.clear();
        m_nodes.clear();
    });
    connect(m_graph, &Graph::graphAboutToBeDeleted,
            this, &QObject::deleteLater);
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

std::unique_ptr<QMenu>
GraphScene::createSceneMenu(QPointF scenePos)
{
// (adapted)
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    auto menuPtr = std::make_unique<QMenu>();
    auto* menu = menuPtr.get();

    // Add filterbox to the context menu
    auto* txtBox = new QLineEdit(menu);
    txtBox->setPlaceholderText(QStringLiteral(" Filter"));
    txtBox->setClearButtonEnabled(true);

    // set the focus to allow text inputs
    QTimer::singleShot(0, txtBox, SIGNAL(setFocus()));

    auto* txtBoxAction = new QWidgetAction(menu);
    txtBoxAction->setDefaultWidget(txtBox);

    txtBox->setMinimumHeight(txtBox->sizeHint().height());

    auto* buttonAction = new QAction(menu);
    buttonAction->setText(tr("Add Comment"));
    buttonAction->setIcon(gt::gui::icon::comment());

    // 1.
    menu->addAction(txtBoxAction);
    menu->addAction(buttonAction);

    // Add result treeview to the context menu
    QTreeWidget* treeView = new QTreeWidget(menu);
    treeView->header()->close();
    treeView->setFrameShape(QFrame::NoFrame);

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
            gtWarning() << tr("Failed to create new node of type %1")
                               .arg(item->text(0));
            return;
        }
        node->setPos(scenePos);

        auto cmd = gtApp->makeCommand(m_graph, tr("Append node '%1'")
                                                   .arg(node->caption()));
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
// SPDX-SnippetEnd

    connect(buttonAction, &QAction::triggered, menu, [this, menu, scenePos]() {
        auto* commentGroup = GuiData::accessCommentGroup(graph());
        if (!commentGroup) return;
        
        auto commentPtr = std::make_unique<CommentData>();
        commentPtr->setPos(scenePos);

        auto cmd = gtApp->makeCommand(m_graph, tr("Append comment '%1'")
                                                   .arg(commentPtr->objectName()));
        Q_UNUSED(cmd);
        
        CommentData* comment = commentGroup->appendComment(std::move(commentPtr));
        if (!comment) return;

        // start editing comment
        auto iter = std::find_if(m_comments.begin(),
                                 m_comments.end(),
                                 Impl::findObjectById(comment));
        if (iter == m_comments.end()) return;

        iter->object->startEditing();
        menu->close();
    });

    return menuPtr;
}

void
GraphScene::alignObjectsToGrid()
{
    auto items = this->selectedItems();
    if (items.empty())
    {
        items = this->items();
        if (items.empty()) return;
    }

    auto cmd = gtApp->makeCommand(m_graph, tr("Align selection to grid"));
    Q_UNUSED(cmd);

    for (QGraphicsItem* item : qAsConst(items))
    {
        if (auto* object = graphics_cast<InteractableGraphicsObject*>(item))
        {
            object->alignToGrid();
        }
    }
}

void
GraphScene::deleteSelectedObjects()
{
    /// std::erase_if wrapper
    static auto const eraseByFlag = [](QList<QGraphicsItem*>& selected,
                                       GraphicsObject::DeletableFlag flag){
        return std::partition(selected.begin(),
                              selected.end(),
                              [flag](QGraphicsItem* item){
            auto* obj = graphics_cast<GraphicsObject*>(item);
            return obj && obj->deletableFlag() != flag;
        });
    };

    /// create popus for certain objects to notify that these are not deletable
    static auto const createPopups = [](auto begin,
                                        auto end,
                                        GraphScene* scene,
                                        QString const& text){
        PopupItem::clearActivePopups();

        // create popup to notify that certain nodes are not deletable
        for (QGraphicsItem* item : utils::makeIterable(begin, end))
        {
            auto* popup = PopupItem::addPopupItem(*scene, text, std::chrono::seconds{1});

            constexpr int yoffset = 5;
            // apply cursor position
            QPointF pos = item->pos() + item->boundingRect().center();
            pos.ry() += 0.5 * item->boundingRect().height() + yoffset;
            pos.rx() -= popup->boundingRect().width() * 0.5;
            popup->setPos(pos);
        }
    };

    auto selected = selectedItems();

    // handle non deletable objects
    auto beginErase = eraseByFlag(selected, GraphicsObject::NotDeletable);

    int count = std::distance(beginErase, selected.end());
    if (count > 0)
    {
        createPopups(beginErase, selected.end(), this,
                    tr("Selected object is not deletable!"));
        selected.erase(beginErase, selected.end());
    }

    // handle non bulk deletable objects
    beginErase = eraseByFlag(selected, GraphicsObject::NotBulkDeletable);

    count = std::distance(beginErase, selected.end());
    if (count > 0 && count != selected.size())
    {
        createPopups(beginErase, selected.end(), this,
                    tr("Selected object is not bulk deletable!"));
        selected.erase(beginErase, selected.end());
    }

    if (selected.empty()) return;

    // sort selected objects by delete priority (e.g. connections before nodes)
    std::sort(selected.begin(), selected.end(), [](QGraphicsItem* left,
                                                   QGraphicsItem* right){
        auto* l = graphics_cast<GraphicsObject*>(left);
        auto* r = graphics_cast<GraphicsObject*>(right);
        return l && r && l->deleteOrdering() < r->deleteOrdering();
    });

    // perform deletion
    auto modification = graph().modify();
    auto cmd = gtApp->makeCommand(m_graph, tr("Delete graphics object%1 from graph %2")
                                               .arg(selected.size() == 1 ? "":"s",
                                                    relativeNodePath(*m_graph)));
    Q_UNUSED(modification);
    Q_UNUSED(cmd);
    for (QGraphicsItem* item : utils::makeIterable(selected))
    {
        if (auto* obj = graphics_cast<GraphicsObject*>(item))
        {
            obj->deleteObject();
        }
    }
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
GraphScene::selectAll()
{
    auto const& objects = Impl::findItems<GraphicsObject*>(*this);
    for (GraphicsObject* object : objects)
    {
        object->setSelected(true);
    }
}

void
GraphScene::keyPressEvent(QKeyEvent* event)
{
    // perform keyevent on node
    auto const& selected = Impl::findSelectedItems<NodeGraphicsObject*>(*this);

    if (selected.size() != 1) return GtGraphicsScene::keyPressEvent(event);

    NodeGraphicsObject* o = selected.front();

    assert(o);
    assert(event);
    event->setAccepted(false);

    gt::gui::handleObjectKeyEvent(*event, o->node());

    if (!event->isAccepted()) GtGraphicsScene::keyPressEvent(event);
}

void
GraphScene::onPortContextMenu(NodeGraphicsObject* object, PortId port)
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

    auto& conModel = graph().connectionModel();
    auto connections = conModel.iterateConnections(node->id(), port);

    QAction* deleteAction = menu.addAction(tr("Remove all connections"));
    deleteAction->setEnabled(!connections.empty());
    deleteAction->setIcon(gt::gui::icon::chainOff());

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == deleteAction)
    {
        auto change = graph().modify();
        Q_UNUSED(change);

        QList<GtObject*> objects;
        std::transform(connections.begin(), connections.end(),
                       std::back_inserter(objects), [this](ConnectionId conId){
            return graph().findConnection(conId);
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
GraphScene::onObjectContextMenu(InteractableGraphicsObject* object)
{
    assert(object);

    if (!object->isSelected()) clearSelection();
    object->setSelected(true);

    auto selected = Impl::findSelectedItems<InteractableGraphicsObject*>(*this);
    assert(!selected.empty());

    auto selectedNodes =
        graphics_cast<NodeGraphicsObject*>(object) ?
            Impl::findItems<NodeGraphicsObject*>(*this, selected) :
            QVector<NodeGraphicsObject*>{};

    // create menu
    QMenu menu;

    bool someCollapsed = std::any_of(selected.begin(),
                                     selected.end(),
                                     Impl::isCollapsed);

    bool someUncollapsed = std::any_of(selected.begin(),
                                       selected.end(),
                                       Impl::negate(Impl::isCollapsed));

    QAction* collapseAction = menu.addAction(tr("Collapse selected Objects"));
    collapseAction->setIcon(gt::gui::icon::triangleUp());
    collapseAction->setVisible(someUncollapsed);

    QAction* uncollapseAction = menu.addAction(tr("Uncollapse selected Objects"));
    uncollapseAction->setIcon(gt::gui::icon::triangleDown());
    uncollapseAction->setVisible(someCollapsed);

    bool areNodesSelected = !selectedNodes.empty();
    Node* selectedNode = areNodesSelected ? &selectedNodes.front()->node() : nullptr;
    Graph* selectedGraphNode = NodeUI::toGraph(selectedNode);

    bool allDeletable = std::all_of(selectedNodes.begin(),
                                    selectedNodes.end(),
                                    Impl::isDefaultDeletable);

    QAction* ungroupAction = menu.addAction(tr("Expand Subgraph"));
    ungroupAction->setIcon(gt::gui::icon::stretch());
    ungroupAction->setEnabled(allDeletable);
    ungroupAction->setVisible(selectedGraphNode && selectedNodes.size() == 1);

    QAction* groupAction = menu.addAction(tr("Group selected Nodes"));
    groupAction->setIcon(gt::gui::icon::select());
    groupAction->setEnabled(allDeletable);
    groupAction->setVisible(areNodesSelected);

    menu.addSeparator();

    QAction* deleteAction = menu.addAction(tr("Delete selected Objects"));
    deleteAction->setIcon(gt::gui::icon::delete_());
    deleteAction->setEnabled(allDeletable);
    deleteAction->setShortcut(QKeySequence::Delete);

    // add custom object menu
    if (selected.size() == 1)
    {
        deleteAction->setVisible(false);
        selected.front()->setupContextMenu(menu);
    }

    QAction* triggered = menu.exec(QCursor::pos());
    if (triggered == deleteAction)
    {
        return deleteSelectedObjects();
    }
    if (triggered == collapseAction ||
        triggered == uncollapseAction)
    {
        bool doCollapse = (triggered == collapseAction);
        return Impl::collapseObjects(*this, selected, doCollapse);
    }
    if (triggered == groupAction)
    {
        return groupSelection();
    }
    if (triggered == ungroupAction)
    {
        return expandSubgraph(selectedGraphNode);
    }
}

void
GraphScene::groupSelection()
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

    auto selectedObjects= Impl::findSelectedItems<InteractableGraphicsObject*>(*this);
    if (selectedObjects.empty()) return;

    QVector<ObjectUuid> selection;

    std::transform(selectedObjects.begin(),
                   selectedObjects.end(),
                   std::back_inserter(selection),
                   [](InteractableGraphicsObject* object){
        assert(object);
        return object->objectUuid();
    });

    // create undo command
    auto appCmd = gtApp->makeCommand(m_graph, tr("Create group node '%1'")
                                                  .arg(groupNodeName));

    auto restoreCmd = gt::finally([&appCmd](){
        appCmd.finalize();
        gtApp->undoStack()->undo();
    });

    // perform grouping
    if (utils::groupObjects(graph(), groupNodeName, selection))
    {
        restoreCmd.clear();
    }
}

void
GraphScene::expandSubgraph(Graph* groupNode)
{
    assert(groupNode);

    // create undo command
    auto appCmd = gtApp->makeCommand(m_graph, tr("Expand group node '%1'").arg(groupNode->caption()));

    auto restoreCmd = gt::finally([&appCmd](){
        appCmd.finalize();
        gtApp->undoStack()->undo();
    });

    // perform expansion
    if (utils::expandSubgraph(std::unique_ptr<Graph>{groupNode}))
    {
        restoreCmd.clear();
    }
}

void
GraphScene::beginMoveCommand(InteractableGraphicsObject* sender, QPointF diff)
{
    if (!m_objectMoveCmd.isValid())
    {
        auto const& selection = Impl::findSelectedItems<NodeGraphicsObject const*>(*this);

        QString const txt = selection.empty() ?
                                tr("Objects moved") :
                                selection.size() > 1 ?
                                    tr("Nodes moved") :
                                    tr("Node '%1' moved")
                                        .arg(relativeNodePath(selection.at(0)->node()));
        m_objectMoveCmd = gtApp->startCommand(m_graph, txt);
    }
}

void
GraphScene::endMoveCommand(InteractableGraphicsObject* sender)
{
    // nodes have not been moved
    if (!m_objectMoveCmd.isValid()) return;

    // finish command
    gtApp->endCommand(m_objectMoveCmd);
    m_objectMoveCmd = {};
}

void
GraphScene::onNodeAppended(Node* node)
{
    static NodeUI defaultUI;
    assert(node);

    NodeUI* ui = qobject_cast<NodeUI*>(gtApp->defaultObjectUI(node));
    if (!ui) ui = &defaultUI;

    auto entity = make_unique_qptr<NodeGraphicsObject, DirectDeleter>(
        *this, *m_sceneData, *node, *ui
    );

    // connect signals
    connect(entity, &NodeGraphicsObject::portContextMenuRequested,
            this, &GraphScene::onPortContextMenu);
    connect(entity, &NodeGraphicsObject::contextMenuRequested,
            this, &GraphScene::onObjectContextMenu);

    connect(entity, &InteractableGraphicsObject::objectShifted,
            this, &GraphScene::beginMoveCommand, Qt::DirectConnection);
    connect(entity, &InteractableGraphicsObject::objectMoved,
            this, &GraphScene::endMoveCommand, Qt::DirectConnection);

    connect(entity, &NodeGraphicsObject::nodeDoubleClicked,
            this, &GraphScene::onNodeDoubleClicked, Qt::DirectConnection);

    connect(entity, &NodeGraphicsObject::makeDraftConnection,
            this, &GraphScene::onMakeDraftConnection,
            Qt::DirectConnection);

    auto* ptr = entity.get();

    // append to map
    m_nodes.push_back({node->id(), std::move(entity)});

    emit objectAdded(ptr, QPrivateSignal());
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
GraphScene::onNodeDoubleClicked(NodeGraphicsObject* sender)
{
    // TODO: move into NodeGraphicsObject?, see core issue #1315
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
    auto* inNode  = nodeObject(conId.inNodeId);
    assert(inNode);

    auto* outNode = nodeObject(conId.outNodeId);
    assert(outNode);

    auto entity = convert_to_unique_qptr<DirectDeleter>(
        ConnectionGraphicsObject::makeConnection(*this, *con, *outNode, *inNode)
    );
    entity->setConnectionShape(m_connectionShape);

    // append to map
    m_connections.push_back({conId, std::move(entity)});

    // update in and out node
    inNode->update();
    outNode->update();
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

void
GraphScene::onFinalizeDraftConnection(ConnectionId conId)
{
    Impl::clearHighlights(*this);

    if (conId.isDraft() || !graph().canAppendConnections(conId)) return;

    auto cmd = gtApp->makeCommand(&graph(), tr("Append %1").arg(toString(conId)));
    Q_UNUSED(cmd);

    graph().appendConnection(std::make_unique<Connection>(conId));
}

void
GraphScene::onCommentAppended(CommentData* comment)
{
    assert(comment);

    auto entity = make_unique_qptr<CommentGraphicsObject, DirectDeleter>(*this, graph(), *comment, sceneData());

    connect(entity, &InteractableGraphicsObject::objectShifted,
            this, &GraphScene::beginMoveCommand, Qt::DirectConnection);
    connect(entity, &InteractableGraphicsObject::objectMoved,
            this, &GraphScene::endMoveCommand, Qt::DirectConnection);

    connect(entity, &CommentGraphicsObject::contextMenuRequested,
            this, &GraphScene::onObjectContextMenu, Qt::DirectConnection);

    auto* ptr = entity.get();

    m_comments.push_back({comment->uuid(), std::move(entity)});

    emit objectAdded(ptr, QPrivateSignal());
}

void
GraphScene::onCommentDeleted(CommentData* comment)
{
    auto iter = std::find_if(m_comments.begin(),
                             m_comments.end(),
                             Impl::findObjectById(comment));
    if (iter == m_comments.end())
    {
        gtError() << utils::logId(this)
                  << tr("Failed to remove comment:") << (void*)comment;
        return;
    }

    m_comments.erase(iter);
}
