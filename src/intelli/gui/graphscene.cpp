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

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

#include <QGraphicsItem>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QMenuBar>

#include <QLineEdit>
#include <QWidgetAction>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>

using namespace intelli;

template <typename T>
inline auto makeCopy(T& obj) {
    std::unique_ptr<GtObject> tmp{obj.copy()};
    return gt::unique_qobject_cast<std::remove_const_t<T>>(std::move(tmp));
};

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

}; // struct Impl

GraphAdapterModel& initGraph(Graph& graph)
{
    auto* model = graph.executionModel();

    if (!model) model = graph.makeExecutionModel();
    else if (model->mode() == GraphExecutionModel::ActiveModel) model->reset();

    if (graph.isActive()) model->autoEvaluate().detach();

    return *new GraphAdapterModel(graph);
}

GraphScene::GraphScene(Graph& graph) :
    QtNodes::BasicGraphicsScene(initGraph(graph), &graph),
    m_graph(&graph)
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
    connect(this, &QtNodes::BasicGraphicsScene::nodeClicked,
            this, [this](QtNodes::NodeId qnodeId){
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
    gtTrace().verbose() << __FUNCTION__;

    if (!m_graph) return;

    auto* model = m_graph->executionModel();
    if (!model) return;

    model->disableAutoEvaluation();
}

void
GraphScene::autoEvaluate(bool enable)
{
    auto* model = m_graph->makeExecutionModel();

    enable ? (void)model->autoEvaluate().detach() :
                   model->disableAutoEvaluation();
}

bool
GraphScene::isAutoEvaluating()
{
    auto* model = m_graph->makeExecutionModel();

    return model->isAutoEvaluating();
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
GraphScene::deleteSelectedObjects()
{
    auto const& selected = Impl::findSelectedItems(*this);
    if (selected.empty()) return;

    GtObjectList objects;
    Impl::findConnections(*m_graph, selected.connections, objects);
    Impl::findNodes(*m_graph, selected.nodes, objects, true);

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

    gtTrace().verbose() << __FUNCTION__;

    // only duplicate internal connections
    int iter = 0;
    foreach (ConnectionId conId, selected.connections)
    {
        if (!selected.nodes.contains(conId.inNodeId) ||
            !selected.nodes.contains(conId.outNodeId))
        {
            selected.connections.removeAt(iter);
            continue;
        }
        iter++;
    }

    QList<Node const*> nodes;
    QList<Connection const*> connections;
    Impl::findConnections(*m_graph, selected.connections, connections);
    Impl::findNodes(*m_graph, selected.nodes, nodes);

    iter = 0;
    foreach (Node const* node, nodes)
    {
        if (node->nodeFlags() & NodeFlag::Unique)
        {
            nodes.removeAt(iter);
            continue;
        }
        iter++;
    }


    // at least one node should be selected
    if (nodes.empty()) return false;

    // append nodes and connections to dummy graph
    Graph dummy;
    for (auto* node : qAsConst(nodes))      dummy.appendNode(makeCopy(*node));
    for (auto* con : qAsConst(connections)) dummy.appendConnection(makeCopy(*con));

    QApplication::clipboard()->setText(dummy.toMemento().toByteArray());

    cleanup.clear();
    return true;
}

void
GraphScene::pasteObjects()
{
    gtTrace().verbose() << __FUNCTION__;

    auto text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    QDomDocument doc;
    if (!doc.setContent(text)) return;

    // restore objects
    GtObjectMemento mem(doc.documentElement());
    auto dummy = gt::unique_qobject_cast<Graph>(mem.toObject(*gtObjectFactory));
    if (!dummy) return;

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
    auto nodes = Impl::findItems<QtNodes::NodeGraphicsObject*>(*this);
    auto iter = 0;
    foreach (auto* node, nodes)
    {
        auto nodeId = node->nodeId();
        if (!newNodeIds.contains(NodeId(nodeId)))
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
        auto connections = Impl::findItems<QtNodes::ConnectionGraphicsObject*>(*this);
        iter = 0;
        foreach (auto* con, connections)
        {
            auto conId = con->connectionId();
            if (!newNodeIds.contains(NodeId(conId.inNodeId)) ||
                !newNodeIds.contains(NodeId(conId.outNodeId)))
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
    QVector<Node*> nodes;
    Impl::findNodes(*m_graph, selectedNodes(), nodes);

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
    if (auto* node = m_graph->findNode(NodeId::fromValue(nodeId)))
    {
        emit gtApp->objectSelected(node);
    }
}

void
GraphScene::onNodeDoubleClicked(QtNodes::NodeId nodeId)
{
    if (auto* node = m_graph->findNode(NodeId::fromValue(nodeId)))
    {
        gt::gui::handleObjectDoubleClick(*node);
    }
}

void
GraphScene::onWidgetResized(QtNodes::NodeId nodeId, QSize size)
{
    if (auto* node = m_graph->findNode(NodeId::fromValue(nodeId)))
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

    auto* node = m_graph->findNode(NodeId::fromValue(nodeId));
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

    PortId portId = m_graph->portId(nodeId, type, idx);

    QList<GtObject*> connections;
    Impl::findConnections(*m_graph, m_graph->findConnections(nodeId, portId), connections);

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
        if (Node* node = m_graph->findNode(id))
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
    Impl::findNodes(*m_graph, nodeIds, nodes);

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
        if (!m_graph->findNode(nodeId)) continue;

        // check connections
        for (ConnectionId conId : m_graph->findConnections(nodeId))
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

    // sort in and out going connections to avoid crossing connections
    auto const sortByPortIndex = [this](ConnectionId a, ConnectionId b){
        Node* nA = m_graph->findNode(a.inNodeId);
        Node* nB = m_graph->findNode(b.inNodeId);
        assert(nA);
        assert(nB);
        return nA->portIndex(nA->portType(a.inPort), a.inPort) <
               nB->portIndex(nB->portType(b.inPort), b.inPort);
    };

    std::sort(connectionsIn.begin(), connectionsIn.end(),
              [=](ConnectionId a, ConnectionId b){
        return sortByPortIndex(a, b);
    });
    std::sort(connectionsOut.begin(), connectionsOut.end(),
              [=](ConnectionId a, ConnectionId b){
        return sortByPortIndex(a.reversed(), b.reversed());
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
    std::vector<QString> dtypeIn;
    for (ConnectionId conId : connectionsIn)
    {
        auto typeId = getTypeId(conId);
        if (typeId.isEmpty()) return;

        dtypeIn.push_back(std::move(typeId));
    }

    // find datatypes for output provider
    std::vector<QString> dtypeOut;
    for (ConnectionId conId : connectionsOut)
    {
        auto typeId = getTypeId(conId.reversed());
        if (typeId.isEmpty()) return;

        dtypeOut.push_back(std::move(typeId));
    }

    // preprocess selected nodes
    std::vector<Node*> selectedNodes;
    Impl::findNodes(*m_graph, selectedNodeIds, selectedNodes);

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
        gtError() << tr("Failed to create group node! "
                        "(Invalid input or output provider)");
        return;
    }

    inputProvider->setPos(inputProvider->pos() - offset);
    outputProvider->setPos(outputProvider->pos() + offset);

    for (QString const& typeId : dtypeIn ) inputProvider->insertPort(typeId);
    for (QString const& typeId : dtypeOut) outputProvider->insertPort(typeId);

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

    auto const makeConnections = [this, groupNode](ConnectionId conId, auto* provider, PortIndex& index, PortType type){
        // create connection in parent graph
        ConnectionId newCon = conId;
        newCon.inNodeId = groupNode->id();
        newCon.inPort   = groupNode->portId(type, index);

        // create connection in subgraph
        conId.outNodeId = provider->id();
        conId.outPort   = provider->portId(invert(type), index);

        assert(newCon.isValid());
        assert(conId .isValid());

        m_graph  ->appendConnection(std::make_unique<Connection>(type == PortType::Out ? newCon.reversed() : newCon));
        groupNode->appendConnection(std::make_unique<Connection>(type == PortType::Out ? conId.reversed()  : conId ));

        index++;
    };

    // move input connections
    PortIndex index{0};
    for (ConnectionId conId : connectionsIn)
    {
        makeConnections(conId, inputProvider, index, PortType::In);
    }

    // move output connections
    index = PortIndex{0};
    for (ConnectionId conId : connectionsOut)
    {
        makeConnections(conId.reversed(), outputProvider, index, PortType::Out);
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
