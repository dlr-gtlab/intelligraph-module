/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/graphscene.h"

#include "intelli/connection.h"
#include "intelli/graphexecmodel.h"
#include "intelli/nodefactory.h"
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
#include <gt_objectio.h>
#include <gt_objectfactory.h>

#include <gt_logging.h>

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QMenuBar>
#include <QLineEdit>
#include <QWidgetAction>
#include <QTreeWidget>
#include <QHeaderView>
#include <QGraphicsView>
#include <QTimer>

using namespace intelli;

template <typename T>
inline auto makeCopy(T& obj) {
    std::unique_ptr<GtObject> tmp{obj.copy()};
    return gt::unique_qobject_cast<std::remove_const_t<T>>(std::move(tmp));
};

struct GraphScene::Impl
{

struct SelectedItems
{
    QVector<NodeGraphicsObject*> nodes;
    QVector<ConnectionGraphicsObject*> connections;

    bool empty() const { return nodes.empty() && connections.empty(); }
};


enum SelectionFilter
{
    NoFilter,
    NodesOnly,
    ConnectionsOnly
};

static SelectedItems findSelectedItems(GraphScene& scene, SelectionFilter filter = NoFilter)
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

template<typename T>
static QVector<T> findItems(GraphScene& scene)
{
    QVector<T> items;

    for (auto* item : scene.items())
    {
        if (auto* obj = qgraphicsitem_cast<T>(item))
        {
            items << obj;
        }
    }
    return items;
}

}; // struct Impl

GraphScene::GraphScene(Graph& graph) :
    m_graph(&graph)
{
    reset();
}

GraphScene::~GraphScene()
{
    if (!m_graph) return;

    auto* model = m_graph->executionModel();
    if (!model) return;

    model->disableAutoEvaluation();
}

void
GraphScene::reset()
{
    beginReset();
    endReset();
}

void
GraphScene::beginReset()
{
    disconnect(m_graph);
}

void
GraphScene::endReset()
{
    m_nodes.clear();

    auto* model = m_graph->executionModel();
    if (!model) model = m_graph->makeExecutionModel();
    else if (model->mode() == GraphExecutionModel::ActiveModel) model->reset();

    auto const& nodes = graph().nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }
    auto const& connections = graph().connections();
    for (auto* con : connections)
    {
        onConnectionAppended(con);
    }

    connect(m_graph, &Graph::nodeAppended, this, &GraphScene::onNodeAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::nodeDeleted, this, &GraphScene::onNodeDeleted, Qt::DirectConnection);

    connect(m_graph, &Graph::connectionAppended, this, &GraphScene::onConnectionAppended, Qt::DirectConnection);
    connect(m_graph, &Graph::connectionDeleted, this, &GraphScene::onConnectionDeleted, Qt::DirectConnection);

    connect(model, &GraphExecutionModel::nodeEvalStateChanged,
            this, &GraphScene::onNodeEvalStateChanged, Qt::DirectConnection);

    if ( m_graph->isActive()) model->autoEvaluate().detach();
}

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

NodeGraphicsObject*
GraphScene::nodeObject(NodeId nodeId)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
                             [nodeId](NodeEntry const& e){ return e.nodeId == nodeId; });
    if (iter == m_nodes.end())
    {
        return nullptr;
    }
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
                             [conId](ConnectionEntry const& e){ return e.conId == conId; });
    if (iter == m_connections.end())
    {
        return nullptr;
    }
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
}

void
GraphScene::setConnectionShape(ConnectionGraphicsObject::ConnectionShape shape)
{
    m_connectionShape = shape;
    if (m_draft)
    {
        m_draft.connection->setConnectionShape(shape);
    }
    for (auto& con : m_connections)
    {
        con.object->setConnectionShape(shape);
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

    auto const containsNode = [&selected](NodeId nodeId){
        return std::find_if(selected.nodes.begin(), selected.nodes.end(), [nodeId](NodeGraphicsObject* o){
            return o->nodeId() == nodeId;
        }) != selected.nodes.end();
    };

    // only duplicate internal connections
    int iter = 0;
    foreach (ConnectionGraphicsObject* o, selected.connections)
    {
        auto con = o->connectionId();
        if (!containsNode(con.inNodeId) || !containsNode(con.outNodeId))
        {
            selected.connections.removeAt(iter);
            continue;
        }
        iter++;
    }

    iter = 0;
    foreach (NodeGraphicsObject const* node, selected.nodes)
    {
        if (node->node().nodeFlags() & NodeFlag::Unique)
        {
            selected.nodes.removeAt(iter);
            continue;
        }
        iter++;
    }

    // at least one node should be selected
    if (selected.nodes.empty()) return false;

    // append nodes and connections to dummy graph
    Graph dummy;
    for (auto* o : qAsConst(selected.nodes)) dummy.appendNode(makeCopy(o->node()));
    for (auto* o : qAsConst(selected.connections)) dummy.appendConnection(std::make_unique<Connection>(o->connectionId()));

    QApplication::clipboard()->setText(dummy.toMemento().toByteArray());

    cleanup.clear();
    return true;
}

void
GraphScene::pasteObjects()
{
    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    QDomDocument doc;
    if (!doc.setContent(text)) return;

    // restore objects
    GtObjectMemento mem(doc.documentElement());
    auto dummy = gt::unique_qobject_cast<Graph>(mem.toObject(*gtObjectFactory));
    if (!dummy) return;

    dummy->newUuid(true);
    auto const& srcNodes = dummy->nodes();
    auto const& srcConnections = dummy->connections();

    if (srcNodes.empty()) return;

    // make unique
    std::vector<std::unique_ptr<Node>> uniqueNodes;
    std::vector<std::unique_ptr<Connection>> uniqueConnections;
    uniqueNodes.reserve(srcNodes.size());
    uniqueConnections.reserve(srcConnections.size());

    std::transform(srcNodes.begin(), srcNodes.end(),
                   std::back_inserter(uniqueNodes), [](Node* node){
        return std::unique_ptr<Node>{node};
    });
    std::transform(srcConnections.begin(), srcConnections.end(),
                   std::back_inserter(uniqueConnections), [](Connection* con){
        return std::unique_ptr<Connection>{con};
    });

    // shift node positions
    constexpr QPointF offset{50, 50};

    for (auto& node : srcNodes)
    {
        node->setPos(node->pos() + offset);
    }

    auto cmd = gtApp->startCommand(m_graph, tr("Paste objects"));
    auto cleanup = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    // append objects
    auto newNodeIds = m_graph->appendObjects(uniqueNodes, uniqueConnections);
    if (newNodeIds.size() != srcNodes.size())
    {
        gtWarning() << tr("Pasting selection failed!");
        return;
    }

    // find affected graphics objects and set selection
    auto nodes = Impl::findItems<NodeGraphicsObject*>(*this);
    auto iter = 0;
    foreach (auto* node, nodes)
    {
        NodeId nodeId = node->nodeId();
        if (!newNodeIds.contains(nodeId))
        {
            nodes.removeAt(iter);
            continue;
        }
        iter++;
    }

    clearSelection();
    for (auto* item : qAsConst(nodes)) item->setSelected(true);

    if (!srcConnections.empty())
    {
        auto connections = Impl::findItems<ConnectionGraphicsObject*>(*this);
        iter = 0;
        foreach (auto* con, connections)
        {
            auto conId = con->connectionId();
            if (!newNodeIds.contains(conId.inNodeId) ||
                !newNodeIds.contains(conId.outNodeId))
            {
                connections.removeAt(iter);
                continue;
            }
            iter++;
        }

        for (auto* item : qAsConst(connections)) item->setSelected(true);
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
    if (m_draft)
    {
        PortType type = m_draft.connection->connectionId().inNodeId.isValid() ? PortType::Out : PortType::In;
        m_draft.connection->setEndPoint(type, event->scenePos());
        return event->accept();
    }

    return QGraphicsScene::mouseMoveEvent(event);
}

void
GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_draft)
    {
        auto pos = event->scenePos();

        ConnectionId conId = m_draft.connection->connectionId();
        bool reverse = conId.inNodeId.isValid();
        if (reverse) conId.reverse();

        m_draft.connection->ungrabMouse();
        m_draft.connection.reset();

        constexpr QPointF offset{5, 5};
        QRectF rect{pos - offset, pos + offset};

        auto const& items = this->items(rect);
        for (auto* item : items)
        {
            gtDebug() << static_cast<QGraphicsObject*>(item);
        }

        for (auto* item : items)
        {
            auto* object = qgraphicsitem_cast<NodeGraphicsObject*>(item);
            if (!object) continue;

            auto hit = object->geometry().portHit(object->mapFromScene(rect).boundingRect());
            if (!hit) break;

            conId.inNodeId = object->nodeId();
            conId.inPort   = hit.port;
            assert(conId.isValid());

            if (reverse) conId.reverse();

            if (!m_graph->canAppendConnections(conId)) break;

            auto cmd = gtApp->makeCommand(m_graph, tr("Append %1").arg(toString(conId)));
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

    QList<GtObject*> connections;
    {
        auto tmp = m_graph->findConnections(node->id(), port);
        std::transform(tmp.begin(), tmp.end(), std::back_inserter(connections),
                       [this](ConnectionId conId){
            auto* o = m_graph->findConnection(conId);
            assert(o);
            return o;
        });
    }

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
GraphScene::onNodeContextMenu(NodeGraphicsObject* object, QPointF pos)
{
    assert(object);

    auto* node = &object->node();

    if (!object->isSelected()) clearSelection();
    object->setSelected(true);

    // retrieve selected nodes
    auto selected = Impl::findSelectedItems(*this, Impl::NodesOnly);

    bool allDeletable = std::all_of(selected.nodes.begin(),
                                    selected.nodes.end(),
                                    [](NodeGraphicsObject* o){
        return o->node().objectFlags() & GtObject::ObjectFlag::UserDeletable;
    });

    // create menu
    QMenu menu;

    QAction* groupAct = menu.addAction(tr("Group selected Nodes"));
    groupAct->setIcon(gt::gui::icon::select());
    groupAct->setEnabled(allDeletable);
    groupAct->setVisible(!selected.nodes.empty());

    menu.addSeparator();

    QAction* deleteAct = menu.addAction(tr("Delete selected Nodes"));
    deleteAct->setIcon(gt::gui::icon::delete_());
    deleteAct->setEnabled(allDeletable);

    // add node to selected nodes
    if (selected.nodes.empty())
    {
        gtWarning() << "FUNCTION IS TRIGGERED";
    }

    // add custom object menu
    if (selected.nodes.size() == 1)
    {
        menu.addSeparator();
        gt::gui::makeObjectContextMenu(menu, *node);
        deleteAct->setVisible(false);
    }

    QAction* triggered = menu.exec(QCursor::pos());

    if (triggered == groupAct)
    {
        return makeGroupNode(selected.nodes);
    }
    if (triggered == deleteAct)
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
GraphScene::makeGroupNode(QVector<NodeGraphicsObject*> const& selectedNodeObjects)
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
    QVector<ConnectionId> connectionsInternal, connectionsIn, connectionsOut;

    for (auto const* object : selectedNodeObjects)
    {
        NodeId nodeId = object->nodeId();
        assert (m_graph->findNode(object->nodeId()));

        // check connections
        for (ConnectionId conId : m_graph->findConnections(nodeId))
        {
            auto const findNode = [&conId](NodeGraphicsObject* o){
                return o->nodeId() == conId.inNodeId;
            };

            // if ingoing node is not part of nodes to group it is an outside connection
            if(std::find_if(selectedNodeObjects.begin(), selectedNodeObjects.end(), findNode) == selectedNodeObjects.end())
            {
                connectionsOut.push_back(conId);
                continue;
            }

            // reverse conId so that outNodeId is now inNodeId
            conId.reverse();

            // if outgoing node is not part of nodes to group it is an outside connection
            if(std::find_if(selectedNodeObjects.begin(), selectedNodeObjects.end(), findNode) == selectedNodeObjects.end())
            {
                // revert reverse
                conId.reverse();
                connectionsIn.push_back(conId);
                continue;
            }

            // revert reverse
            conId.reverse();
            // internal connection
            connectionsInternal.push_back(conId);
        }
    }

    // sort in and out going connections to avoid crossing connections
    auto const sortByScenePos = [this](ConnectionId a, ConnectionId b, bool reverse = false){
        auto oA = connectionObject(reverse ? a.reversed() : a);
        auto oB = connectionObject(reverse ? b.reversed() : b);
        assert(oA);
        assert(oB);
        return oA->endPoint(PortType::In).y() < oB->endPoint(PortType::In).y();
    };

    std::sort(connectionsIn.begin(), connectionsIn.end(),
              [=](ConnectionId a, ConnectionId b){
        return sortByScenePos(a, b);
    });
    std::sort(connectionsOut.begin(), connectionsOut.end(),
              [=](ConnectionId a, ConnectionId b){
        return sortByScenePos(a.reversed(), b.reversed(), true);
    });

    auto const getTypeId = [this](ConnectionId conId){
        auto* node = m_graph->findNode(conId.inNodeId);
        assert(node);
        auto* port = node->port(conId.inPort);
        assert(port);

        if (!NodeDataFactory::instance().knownClass(port->typeId))
        {
            gtError() << tr("Failed to create group node! "
                            "(Unkown node datatype '%1', id: %2, port: %3)")
                             .arg(port->typeId, node->caption(), toString(*port));
            return QString{};
        }

        return port->typeId;
    };

    // find datatype for input provider
    QVector<QString> dtypeIn;
    for (ConnectionId conId : connectionsIn)
    {
        auto typeId = getTypeId(conId);
        if (typeId.isEmpty()) return;

        dtypeIn.push_back(std::move(typeId));
    }

    // find datatypes for output provider
    QVector<QString> dtypeOut;
    for (ConnectionId conId : connectionsOut)
    {
        auto typeId = getTypeId(conId.reversed());
        if (typeId.isEmpty()) return;

        dtypeOut.push_back(std::move(typeId));
    }

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
    groupNode->setActive(true);

    // setup input/output provider
    groupNode->initInputOutputProviders();
    auto* inputProvider  = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << tr("Failed to create group node! "
                        "(Invalid input or output provider)");
        return;
    }

    inputProvider->setPos(inputProvider->pos() - offset);
    outputProvider->setPos(outputProvider->pos() + offset);

    for (QString const& typeId : dtypeIn ) inputProvider->insertPort(typeId);
    for (QString const& typeId : dtypeOut) outputProvider->insertPort(typeId);

    QVector<ConnectionId> sharedConnections;

    foreach (ConnectionId conId, connectionsIn)
    {
        auto iter = std::find_if(connectionsIn.begin(), connectionsIn.end(), [conId](ConnectionId other){
            return conId != other && conId.outNodeId == other.outNodeId;
        });
        if (iter == connectionsIn.end()) continue;

        gtDebug() << "SHARED INPUT" << *iter;
    }

    foreach (ConnectionId conId, connectionsOut)
    {
        auto iter = std::find_if(connectionsOut.begin(), connectionsOut.end(), [conId](ConnectionId other){
            return conId != other && conId.outNodeId == other.outNodeId;
        });
        if (iter == connectionsOut.end()) continue;

        gtDebug() << "SHARED OUTPUT" << *iter;
    }

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
            if      (conId.inNodeId  == oldId) conId.inNodeId  = newId;
            else if (conId.outNodeId == oldId) conId.outNodeId = newId;
        }
    }

    // move group to graph
    auto appCmd = gtApp->makeCommand(m_graph, tr("Create group node '%1'").arg(groupNodeName));
    auto modifyCmd = m_graph->modify();

    Q_UNUSED(appCmd);
    Q_UNUSED(modifyCmd);

    if (!m_graph->appendNode(std::move(tmpGraph)))
    {
        gtError() << tr("Failed to create group node! (Invalid group node)");
        return;
    }

    // remove old nodes and connections. Connections must be deleted before
    // appending new connections
    qDeleteAll(selectedNodes);

    auto const makeConnections = [this, groupNode](ConnectionId conId, auto* provider, PortIndex index, PortType type){
        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.inNodeId = groupNode->id();
        newCon.inPort   = groupNode->portId(type, index);

        // create connection in subgraph
        conId.outNodeId = provider->id();
        conId.outPort   = provider->portId(invert(type), index);

        assert(newCon.isValid());
        assert(conId .isValid());

        m_graph->appendConnection(  std::make_unique<Connection>(type == PortType::Out ? newCon.reversed() : newCon));
        groupNode->appendConnection(std::make_unique<Connection>(type == PortType::Out ? conId.reversed()  : conId ));
    };

    // move input connections
    PortIndex index{0};
    for (ConnectionId conId : connectionsIn)
    {
        makeConnections(conId, inputProvider, index++, PortType::In);
    }

    // move output connections
    index = PortIndex{0};
    for (ConnectionId conId : connectionsOut)
    {
        makeConnections(conId.reversed(), outputProvider, index++, PortType::Out);
    }

    // move internal connections
    for (ConnectionId const& conId : connectionsInternal)
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

    auto entity = make_volatile<NodeGraphicsObject, DirectDeleter>(*m_graph, *node, *ui);
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
    connect(entity, &NodeGraphicsObject::nodeGeometryChanged,
            this, &GraphScene::moveConnections, Qt::DirectConnection);

    connect(entity, qOverload<NodeGraphicsObject*, ConnectionId>(&NodeGraphicsObject::makeDraftConnection),
            this,   qOverload<NodeGraphicsObject*, ConnectionId>(&GraphScene::onMakeDraftConnection),
            Qt::DirectConnection);
    connect(entity, qOverload<NodeGraphicsObject*, PortType, PortId>(&NodeGraphicsObject::makeDraftConnection),
            this,   qOverload<NodeGraphicsObject*, PortType, PortId>(&GraphScene::onMakeDraftConnection),
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
GraphScene::onNodeEvalStateChanged(NodeId nodeId)
{
    auto* node = nodeObject(nodeId);
    assert(node);

    auto exec = m_graph->executionModel();
    node->setNodeEvalState(exec->nodeEvalState(nodeId));
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
        auto* port = inNode->port(id);
        entity->setPortTypeId(PortType::In, port->typeId);
    });
    connect(outNode, &Node::portChanged, entity,
            [entity = entity.get(), outNode](PortId id){
        auto* port = outNode->port(id);
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
    auto iter = std::find_if(m_connections.begin(), m_connections.end(),
                             [conId](ConnectionEntry const& e){ return e.conId == conId; });
    if (iter != m_connections.end())
    {
        m_connections.erase(iter);
    }

    // update in and out node
    auto* inNode  = nodeObject(conId.inNodeId);
    assert(inNode);
    inNode->update();

    auto* outNode = nodeObject(conId.outNodeId);
    assert(outNode);
    outNode->update();
}

void
GraphScene::moveConnection(ConnectionGraphicsObject* object, NodeGraphicsObject* node)
{
    assert(object);

    bool isInNode  = !node || node->nodeId() == object->connectionId().inNodeId;
    bool isOutNode = !node || !isInNode;

    if (isInNode)
    {
        moveConnectionPoint(object, PortType::In);
    }
    if (isOutNode)
    {
        moveConnectionPoint(object, PortType::Out);
    }
}

void
GraphScene::moveConnectionPoint(ConnectionGraphicsObject* object, PortType type)
{
    auto const& conId = object->connectionId();

    NodeId nodeId = conId.node(type);
    assert(nodeId != invalid<NodeId>());

    NodeGraphicsObject* nObject = nodeObject(nodeId);
    if (!nObject) return;

    Node& node = nObject->node();

    auto const& geometry = nObject->geometry();

    QRectF portRect = geometry.portRect(type, node.portIndex(type, conId.port(type)));
    QPointF nodePos = nObject->sceneTransform().map(portRect.center());

    QPointF connectionPos = object->sceneTransform().inverted().map(nodePos);

    object->setEndPoint(type, connectionPos);
};

void
GraphScene::moveConnections(NodeGraphicsObject* object)
{
    assert(object);

    auto const& connections = m_graph->findConnections(object->nodeId());

    for (auto const& conId : connections)
    {
        if (ConnectionGraphicsObject* con = connectionObject(conId))
        {
            moveConnection(con, object);
        }
    }
}

void
GraphScene::onMakeDraftConnection(NodeGraphicsObject* object, ConnectionId conId)
{
    assert(!m_draft);
    assert(object);
    assert(conId.isValid());
    assert(conId.inNodeId == object->nodeId());

    // this function is only called if ingoing connection was disconnected
    constexpr PortType type = PortType::In;

    QPointF oldEndPoint;

    // get old end point for draft connection
    {
        ConnectionGraphicsObject* oldCon = connectionObject(conId);
        assert(oldCon);
        oldEndPoint = oldCon->endPoint(type);
    }

    bool success = gtDataModel->deleteFromModel(m_graph->findConnection(conId));
    assert(success);

    // make draft connection form outgoing node
    onMakeDraftConnection(nodeObject(conId.outNodeId), invert(type), conId.outPort);

    // move initial end position of draft connection
    assert(m_draft);
    m_draft.connection->setEndPoint(type, oldEndPoint);
}

void
GraphScene::onMakeDraftConnection(NodeGraphicsObject* object, PortType type, PortId port)
{
    assert (!m_draft);
    assert(object);
    assert(port.isValid());

    ConnectionId conId{
        object->nodeId(),
        port,
        invalid<NodeId>(),
        invalid<PortId>()
    };

    if (type == PortType::In) conId.reverse();

    auto entity = make_volatile<ConnectionGraphicsObject>(conId);
    entity->setConnectionShape(m_connectionShape);
    addItem(entity);
    moveConnectionPoint(entity, type);
    entity->setEndPoint(invert(type), entity->endPoint(type));
    entity->grabMouse();
    m_draft.connection = std::move(entity);
}
