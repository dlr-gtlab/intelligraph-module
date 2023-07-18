/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphscene.h"

#include "gt_intelligraphnodeui.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphdatafactory.h"
#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphmodelmanager.h"

#include "gt_application.h"
#include "gt_command.h"
#include "gt_datamodel.h"
#include "gt_qtutilities.h"
#include "gt_guiutilities.h"
#include "gt_icons.h"
#include "gt_inputdialog.h"
#include "gt_objectmemento.h"
#include "gt_objectfactory.h"
#include "models/gt_intelligraphobjectmodel.h"

#include <gt_logging.h>

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

#include <QGraphicsItem>

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
void findNodes(GtIntelliGraph& graph, In const& in, Out& out)
{
    out.reserve(in.size());

    for (auto nodeId : in)
    {
        if (auto* node = graph.findNode(nodeId))
        {
            out.push_back(node);
        }
    }
}

GtIntelliGraphScene::GtIntelliGraphScene(GtIntelliGraph& graph, QObject* parent) :
    QtNodes::DataFlowGraphicsScene(*graph.makeModelManager()->graphModel(), parent),
    m_data(&graph),
    m_model(static_cast<QtNodes::DataFlowGraphModel*>(&graphModel()))
{
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

    connect(m_model, &QtNodes::DataFlowGraphModel::nodePositionUpdated,
            this, &GtIntelliGraphScene::onNodePositionChanged);
}

void
GtIntelliGraphScene::deleteSelectedObjects()
{
    gtDebug() << __FUNCTION__;

    auto const& items = selectedItems();
    if (selectedItems().empty()) return;

    QVector<QtNodes::NodeId> selectedNodes;
    QVector<QtNodes::ConnectionId> selectedConnections;

    for (auto* item : items)
    {
        if (auto* node = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item))
        {
            selectedNodes << node->nodeId();
            continue;
        }
        if (auto* con = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item))
        {
            selectedConnections << con->connectionId();
            continue;
        }
    }

    GtObjectList objects;
    findConnections(*m_data, selectedConnections, objects);
    findNodes(*m_data, selectedNodes, objects);

    gtDataModel->deleteFromModel(objects);
}

void
GtIntelliGraphScene::duplicateSelectedObjects()
{
    auto selectedNodes = this->selectedNodes();

    gtDebug() << __FUNCTION__;
}

void
GtIntelliGraphScene::copySelectedObjects()
{
    gtDebug() << __FUNCTION__;
}

void
GtIntelliGraphScene::pasteObjects()
{
    gtDebug() << __FUNCTION__;
}

void
GtIntelliGraphScene::onNodePositionChanged(QtNodes::NodeId nodeId)
{
    assert(m_data);

    auto* delegate = m_model->delegateModel<GtIntelliGraphObjectModel>(nodeId);

    if (!delegate || !delegate->node()) return;

    auto position = m_model->nodeData(nodeId, QtNodes::NodeRole::Position);

    if (!position.isValid()) return;

    auto pos = position.toPointF();

    auto* node = delegate->node();

    gtInfo().verbose()
        << tr("Updating node position to") << pos
        << gt::brackets(node->objectName());

    node->setPos(pos);
}

void
GtIntelliGraphScene::onNodeSelected(QtNodes::NodeId nodeId)
{
    assert(m_data);

    if (auto* node = m_data->findNode(nodeId))
    {
        emit gtApp->objectSelected(node);
    }
}

void
GtIntelliGraphScene::onNodeDoubleClicked(QtNodes::NodeId nodeId)
{
    assert(m_data);

    auto* node = m_data->findNode(nodeId);
    if (!node) return;

    gt::gui::handleObjectDoubleClick(*node);
}

void
GtIntelliGraphScene::onPortContextMenu(QtNodes::NodeId nodeId,
                                       QtNodes::PortType type,
                                       QtNodes::PortIndex idx,
                                       QPointF pos)
{
    using PortType  = gt::ig::PortType;
    using PortIndex = gt::ig::PortIndex;

    assert(m_data); assert(m_model);

    auto* node = m_data->findNode(nodeId);
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
    findConnections(*m_data, m_model->allConnectionIds(nodeId), connections);

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
    assert(m_data); assert(m_model);

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
        if (GtIntelliGraphNode* node = m_data->findNode(selectedNodeIds.front()))
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

    auto cmd = gtApp->startCommand(m_data, "bla");
    auto finally = gt::finally([&](){
        gtApp->endCommand(cmd);
    });

    // create group node
    QtNodes::NodeId groupNodeId = m_model->addNode(m_data->modelName());
    auto* groupNode = qobject_cast<GtIntelliGraph*>(m_data->findNode(groupNodeId));
    if (!groupNode || !groupNode->findModelManager())
    {
        gtError() << tr("Failed to create group node! (Invalid group node)");
        return;
    }

    auto groupModel = groupNode->findModelManager()->graphModel();
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

    for (QtNodes::NodeId nodeId : selectedNodeIds)
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

        delete node; //m_data->deleteNode(oldId);

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

