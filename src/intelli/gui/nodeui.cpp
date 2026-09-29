/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/nodeui.h"

#include "intelli/dynamicnode.h"
#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/graphdatamodel.h"
#include "intelli/graphexecmodel.h"

#include "intelli/node/dummy.h"
#include "intelli/node/input/graphuservariablesinput.h"

#include "intelli/gui/icons.h"
#include "intelli/gui/nodeuidata.h"
#include "intelli/gui/nodegeometry.h"
#include "intelli/gui/nodepainter.h"
#include "intelli/gui/graphics/nodeobject.h"
#include "intelli/gui/widgets/graphuservariablesdialog.h"
#include "intelli/gui/widgets/porteditdialog.h"

#include "intelli/private/utils.h"
#include "intelli/private/node_impl.h" // temporary, needed for widget factory

#include <gt_logging.h>

#include <gt_colors.h>
#include <gt_palette.h>
#include <gt_command.h>
#include <gt_inputdialog.h>
#include <gt_application.h>

#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
#define ORDER_PRIORITY(X) (X)
 #define SET_ORDER_PRIORITY(X) .setOrderPriority(X)
#else
 #define ORDER_PRIORITY(X)
 #define SET_ORDER_PRIORITY(X)
#endif

using namespace intelli;

using DeleteAction = std::pair<NodeUI::CustomDeleteFunctor,
                               NodeUI::EnableCustomDeleteFunctor>;

// allows to use variadic arguments
auto const hasInputPorts = [](GtObject* obj, auto ...){
    return (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicInput);
};
auto const hasOutputPorts = [](GtObject* obj, auto ...){
    return (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicOutput);
};

DummyNode*
toDummyNode(GtObject* obj) { return qobject_cast<DummyNode*>(obj); }

DummyNode const*
toConstDummyNode(Node const* obj) { return qobject_cast<DummyNode const*>(obj); }

GraphUserVariablesInputNode*
toUserVariablesNode(GtObject* obj) { return qobject_cast<GraphUserVariablesInputNode*>(obj); }

NodeUI::ActionChainOperator const isDummyNode{toDummyNode};

NodeUI::ActionChainOperator const NodeUI::isNode{NodeUI::toNode};

NodeUI::ActionChainOperator const NodeUI::isGraph{NodeUI::toGraph};

NodeUI::ActionChainOperator const NodeUI::isRootGraph{
    [](GtObject const* obj){
        Graph const* graph = toConstGraph(obj);
        return graph && graph->rootGraph() == graph;
    }
};

NodeUI::ActionChainOperator const NodeUI::isDynamicNode{NodeUI::toDynamicNode};

NodeUI::ActionChainOperator const NodeUI::isNodeActive{
    [](GtObject* obj){
        Node* node = static_cast<Node*>(obj);
        return node && node->isActive();
    }
};

NodeUI::PortActionChainOperator const NodeUI::isInputPort{
    [](Node* node, PortType type, PortIndex index) {
        return node && type == PortType::In && node->ports(type).size() > index;
    }
};

NodeUI::PortActionChainOperator const NodeUI::isOutputPort{
    [](Node* node, PortType type, PortIndex index){
        return node && type == PortType::Out && node->ports(type).size() > index;
    }
};

NodeUI::PortActionChainOperator const NodeUI::isDynamicPort{
    [](Node* obj, PortType type, PortIndex idx){
        if (toDummyNode(obj)) return false;
        if (auto* node = toDynamicNode(obj))
        {
            return node->isDynamicPort(type, idx);
        }
        return false;
    }
};

struct NodeUI::Impl
{
    /// List of custom port actions
    QList<PortUIAction> portActions;

    QList<DeleteAction> deleteActions;
};

NodeUI::NodeUI(Options options) :
    pimpl(std::make_unique<Impl>())
{
    addSeparator(ORDER_PRIORITY(gt::gui::OrderPriority::AfterDeleteAction));

    addCustomDeleteAction(tr("Delete Dummy Node"), deleteDummyNode, toConstDummyNode);

    if (!(options & NoDefaultNodeActions))
    {
        static auto const& category =  QStringLiteral("GtProcessDock");

        if (!(options & NoRenameOption))
        {
            addSeparator(ORDER_PRIORITY(gt::gui::OrderPriority::BeforeRenameAction));

            addSingleAction(tr("Rename"), renameNode)
                .setIcon(gt::gui::icon::rename())
                .setVisibilityMethod(toNode)
                .setVerificationMethod(canRenameNodeObject)
                .setShortCut(gtApp->getShortCutSequence("rename"))
                SET_ORDER_PRIORITY(gt::gui::OrderPriority::RenameAction);

            addSeparator(ORDER_PRIORITY(gt::gui::OrderPriority::AfterRenameAction));
        }

        addSeparator(ORDER_PRIORITY(OrderPriority::BeforeEvaluationActions));

        addSingleAction(tr("Execute once"), executeNode)
            .setIcon(gt::gui::icon::processRun())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("runProcess"), category))
            .setVisibilityMethod(isNode && !isRootGraph && !isDummyNode)
            SET_ORDER_PRIORITY(OrderPriority::EvaluationAction);

        addSingleAction(tr("Set Inactive"), setActive<false>)
            .setIcon(gt::gui::icon::sleep())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("skipProcess"), category))
            .setVisibilityMethod(isNodeActive && !isRootGraph && !isDummyNode)
            SET_ORDER_PRIORITY(OrderPriority::EvaluationAction);

        addSingleAction(tr("Set Active"), setActive<true>)
            .setIcon(gt::gui::icon::sleepOff())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("unskipProcess"), category))
            .setVisibilityMethod(!isNodeActive && !isRootGraph && !isDummyNode)
            SET_ORDER_PRIORITY(OrderPriority::EvaluationAction);

        addSeparator(ORDER_PRIORITY(OrderPriority::AfterEvaluationActions));

        addSingleAction(tr("Edit User Variables..."), editUserVariables)
            .setIcon(gt::gui::icon::variable())
            .setVisibilityMethod(isRootGraph || toUserVariablesNode)
            SET_ORDER_PRIORITY(OrderPriority::CustomAction);

        addSeparator(ORDER_PRIORITY(OrderPriority::AfterCustomActions));

        if (!(options & (NoDynamicPortActions) ))
        {
            addSingleAction(tr("Add In Port"), addDynamicInPort)
                .setIcon(gt::gui::icon::add())
                .setVisibilityMethod(isDynamicNode && !isDummyNode && hasInputPorts)
                SET_ORDER_PRIORITY(OrderPriority::PortAction);

            addSingleAction(tr("Add Out Port"), addDynamicOutPort)
                .setIcon(gt::gui::icon::add())
                .setVisibilityMethod(isDynamicNode && !isDummyNode && hasOutputPorts)
                SET_ORDER_PRIORITY(OrderPriority::PortAction);

            addSeparator(ORDER_PRIORITY(OrderPriority::AfterPortActions));
        }
    }

    if (gtApp && gtApp->devMode())
    {
        addActionGroup(tr("Debug"))
            SET_ORDER_PRIORITY(gt::gui::OrderPriority::Last + 1)
            .setIcon(gt::gui::icon::bug())
            << makeSingleAction(tr("Refresh Node"), [](GtObject* obj){
                    if (auto* node = toNode(obj)) emit node->nodeChanged();
                })
                .setIcon(gt::gui::icon::reload())
                .setVisibilityMethod(isNode)
            << makeSingleAction(tr("Print Graph Debug Information"), [](GtObject* obj){
                    if (auto* graph = toGraph(obj))
                    {
                        QString const& path = relativeNodePath(*graph);
                        gtInfo().nospace() << "Local Connection Model: (" << path << ")";
                        debug(graph->connectionModel());
                        gtInfo().nospace() << "Global Connection Model: (" << path << ")";
                        debug(graph->globalConnectionModel());
                    }
                })
                .setIcon(gt::gui::icon::bug())
                .setVisibilityMethod(isGraph)
            << makeSingleAction(tr("Print Debug Port Information"), [](GtObject* obj){
                    if (auto* node = toNode(obj))
                    {
                        QString const& path = relativeNodePath(*node);
                        gtInfo() << "### Node:" << path << node->uuid()
                                 << gt::brackets(toString(node->id()));
                        gtInfo() << "###  - Inputs:";
                        for (auto const& port : node->ports(PortType::In))
                        {
                            gtInfo() << "###    -> " << port;
                        }
                        gtInfo() << "###  - Outputs:";
                        for (auto const& port : node->ports(PortType::Out))
                        {
                            gtInfo() << "###    -> " << port;
                        }
                        gtInfo() << "###";
                    }
                })
                .setIcon(gt::gui::icon::bug())
                .setVisibilityMethod(isNode)
            << makeSeparator()
            << makeSingleAction(tr("Force Delete"), [](GtObject* obj){
                    if (obj) obj->deleteLater();
                })
                .setIcon(gt::gui::icon::delete_());
    }

    ///////////////////////////// PORT ACTIONS /////////////////////////////////

    if (gtApp && gtApp->devMode())
    {
        addPortAction(tr("Port Info"), [](Node* obj, PortType type, PortIndex idx){
                if (!obj) return;
                PortId portId = obj->portId(type, idx);
                NodePort* port = obj->port(portId);

                gtInfo() << tr("Node '%1' (id: %2), Port: %3")
                                .arg(obj->caption(),
                                     toString(obj->id()),
                                     port ? toString(*port) : "null");
            })
            .setIcon(gt::gui::icon::bug());

        addPortSeparator();
    }

    if (!(options & (NoDefaultPortActions | NoDynamicPortActions)))
    {
        addPortAction(tr("Edit Port"), editDynamicPort)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(isDynamicPort && (isInputPort || isOutputPort) && hasInputPorts);

        addPortAction(tr("Delete Port"), deleteDynamicPort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(isDynamicPort && (isInputPort || isOutputPort)  && hasInputPorts);
    }
}

NodeUI::~NodeUI() = default;

std::unique_ptr<NodePainter>
NodeUI::painter(NodeGraphicsObject const& object,
                NodeGeometry const& geometry) const
{
    return std::make_unique<NodePainter>(object, geometry);
}

std::unique_ptr<NodeGeometry>
NodeUI::geometry(NodeGraphicsObject const& object) const
{
    return std::make_unique<NodeGeometry>(object);
}

std::unique_ptr<NodeUIData>
NodeUI::uiData(Node const& node) const
{
    auto uiData = std::unique_ptr<NodeUIData>(new NodeUIData{});
    uiData->setDisplayIcon(displayIcon(node));
    uiData->setWidgetFactory(centralWidgetFactory(node));
    uiData->setCustomDeleteFunction(customDeleteAction(node));
    return uiData;
}

NodeUI::CustomDeleteFunctor
NodeUI::customDeleteAction(Node const& node) const
{
     auto iter = std::find_if(pimpl->deleteActions.begin(),
                              pimpl->deleteActions.end(),
                              [n = &node](DeleteAction element){
         return element.second(n);
     });
    if (iter == pimpl->deleteActions.end()) return {};
    return iter->first;
}

QIcon
NodeUI::icon(GtObject* obj) const
{
    Node* node = toNode(obj);
    if (!node)
    {
        return gt::gui::icon::objectEmpty();
    }

    if (toDummyNode(obj))
    {
        return gt::gui::colorize(gt::gui::icon::objectUnknown(),
                                 gt::gui::color::warningText());
    }

    QIcon icon = displayIcon(*node);
    if (!icon.isNull())
    {
        return icon;
    }

    return gt::gui::icon::intelli::node();
}

QIcon
NodeUI::displayIcon(Node const& node) const
{
    if (node.nodeFlags() & NodeFlag::Deprecated)
    {
        return gt::gui::icon::warningColorized();
    }
    if (qobject_cast<DummyNode const*>(&node))
    {
        return gt::gui::colorize(gt::gui::icon::questionmark(),
                                 gt::gui::color::warningText);
    }
    return QIcon{};
}

NodeUI::WidgetFactoryFunction
NodeUI::centralWidgetFactory(Node const& node) const
{
    if (!node.pimpl->widgetFactory) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {

        if (!source.pimpl->widgetFactory) return {};

        auto widget = source.pimpl->widgetFactory(source);

        return convertToGraphicsWidget(std::move(widget), object);
    };
}

std::unique_ptr<QGraphicsWidget>
NodeUI::convertToGraphicsWidget(std::unique_ptr<QWidget> widget, NodeGraphicsObject& object)
{
    auto* w = widget.get();
    if (!w) return {};

    auto proxyWidget = std::make_unique<QGraphicsProxyWidget>();
    proxyWidget->setWidget(widget.release());

    /// Update the palette of the widget
    QObject::connect(&object, &NodeGraphicsObject::updateWidgetPalette,
                     w, [o = QPointer<NodeGraphicsObject>(&object),
                         w = QPointer<QWidget>(w)](){
        assert(o);
        assert(w);
        gt::gui::applyThemeToWidget(w);

        QPalette p = w->palette();
        p.setColor(QPalette::Window, o->painter().backgroundColor());
        w->setPalette(p);
    });

    return proxyWidget;
}

QStringList
NodeUI::openWith(GtObject* obj)
{
    return {};
}

PortUIAction&
NodeUI::addPortAction(QString const& actionText, PortActionFunction actionMethod)
{
    pimpl->portActions.append(PortUIAction(actionText, std::move(actionMethod)));
    return pimpl->portActions.back();
}

void
NodeUI::addPortSeparator()
{
    pimpl->portActions.append(PortUIAction{});
}

void
NodeUI::addCustomDeleteAction(QString const& text,
                              CustomDeleteFunctor deleteFunctor,
                              EnableCustomDeleteFunctor enableDeleteFunctor)
{
    pimpl->deleteActions.push_back({ deleteFunctor, enableDeleteFunctor });

    addSingleAction(text, [f = std::move(deleteFunctor)](GtObject* obj) {
            f(qobject_cast<Node*>(obj));
        })
        SET_ORDER_PRIORITY(gt::gui::OrderPriority::DeleteAction)
        .setIcon(gt::gui::icon::delete_())
        .setShortCut(gtApp->getShortCutSequence("delete"))
        .setVisibilityMethod([f = std::move(enableDeleteFunctor)](GtObject* obj) {
            return f(qobject_cast<Node const*>(obj));
        });
}

void
NodeUI::addCustomDeleteAction(CustomDeleteFunctor deleteFunctor,
                              EnableCustomDeleteFunctor enableDeleteFunctor)
{
    return addCustomDeleteAction(
        tr("delete"), std::move(deleteFunctor), std::move(enableDeleteFunctor)
    );
}

Node*
NodeUI::toNode(GtObject* obj)
{
    return qobject_cast<Node*>(obj);
}

Node const*
NodeUI::toConstNode(GtObject const* obj)
{
    return qobject_cast<Node const*>(obj);
}

Graph*
NodeUI::toGraph(GtObject* obj)
{
    return qobject_cast<Graph*>(obj);
}

Graph const*
NodeUI::toConstGraph(GtObject const* obj)
{
    return qobject_cast<Graph const*>(obj);
}

DynamicNode*
NodeUI::toDynamicNode(GtObject* obj)
{
    return qobject_cast<DynamicNode*>(obj);
}

DynamicNode const*
NodeUI::toConstDynamicNode(GtObject const* obj)
{
    return qobject_cast<DynamicNode const*>(obj);;
}

bool
NodeUI::canRenameNodeObject(GtObject* obj)
{
    if (!obj || toDummyNode(obj))
    {
        return false;
    }
    if (auto* node = toNode(obj))
    {
        return !(node->nodeFlags() & Unique);
    }
    return true;
}

void
NodeUI::renameNode(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node && !canRenameNodeObject(node)) return;

    GtInputDialog dialog(GtInputDialog::TextInput);
    dialog.setWindowTitle(tr("Rename Node Object"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(tr("Enter the new node base name."));
    dialog.setInitialTextValue(node->baseObjectName());

    if (dialog.exec())
    {
        auto text = dialog.textValue();
        if (!text.isEmpty())
        {
            auto cmd = gtApp->makeCommand(node,
                                          QStringLiteral("Renaming node '%1' to '%2'")
                                              .arg(relativeNodePath(*node), text));
            Q_UNUSED(cmd);

            node->setCaption(text);
        }
    }
}

void
NodeUI::executeNode(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node) return;

    auto* graph = toGraph(node->parentObject());
    if (!graph) return;

#if 0
    auto* executor = graph->findDirectChild<GraphExecutor*>();
    if (!executor) return;

    auto* dataModel = graph->findDirectChild<GraphDataModel*>();
    if (!dataModel) return;

    dataModel->invalidateNode(node->uuid());

    auto future = executor->evaluateNode(node->id());
    Q_UNUSED(future)
#else
    auto model = GraphExecutionModel::accessExecModel(*graph);
    if (!model) return;

    auto const& nodeUuid = node->uuid();
    model->invalidateNode(nodeUuid);
    model->evaluateNode(nodeUuid).detach();
#endif
}

namespace
{

void
addPort(DynamicNode& node, PortType type)
{
    PortEditDialog::Option option{PortEditDialog::AllowListTypes};

    auto dynOptions = node.dynamicNodeOptions();
    if (dynOptions.testFlag(DynamicNode::NoDefaultListTypes) ||
        dynOptions.testFlag(DynamicNode::ListTypesOnly))
    {
        option = PortEditDialog::NoOption;
    }

    PortEditDialog dialog{
        type,
        option,
        type == PortType::In ?
            node.inputWhitelist() : node.outputWhitelist()
    };
    if (!dialog.exec()) return;

    Node::PortInfo newPort{dialog.typeId()};
    newPort.caption = dialog.caption();
    newPort.captionVisible = dialog.captionVisible();
    newPort.optional = dialog.optional();

    auto cmd = gtApp->makeCommand(
        &node,
        QStringLiteral("Adding an %1put port to conditional node '%2'")
            .arg(type == PortType::In ? "in" : "out",
                 relativeNodePath(node)));
    Q_UNUSED(cmd);

    auto id = (type == PortType::In) ?
                  node.addInPort(std::move(newPort)) :
                  node.addOutPort(std::move(newPort));

    auto* port = node.port(id);
    if (!port)
    {
        gtWarning().verbose() << QObject::tr("Failed to add dynamic port to %1!")
                                     .arg(relativeNodePath(node));
        return;
    }
    gtInfo().verbose() << QObject::tr("Added dynamic port '%1'")
                              .arg(port ? toString(*port) : "N/A");
}

void
editPort(DynamicNode& node, PortType type, NodePort& srcPort)
{
    PortEditDialog::Option option{PortEditDialog::AllowListTypes};

    auto dynOptions = node.dynamicNodeOptions();
    if (dynOptions.testFlag(DynamicNode::NoDefaultListTypes) ||
        dynOptions.testFlag(DynamicNode::ListTypesOnly))
    {
        option = PortEditDialog::NoOption;
    }

    PortEditDialog dialog{
        type,
        option,
        type == PortType::In ?
            node.inputWhitelist() : node.outputWhitelist()
    };
    dialog.setTypeId(srcPort.typeId);
    dialog.setCaption(srcPort.caption);
    dialog.setCaptionVisible(srcPort.captionVisible);
    dialog.setOptional(srcPort.optional);
    if (!dialog.exec()) return;

    auto cmd = gtApp->makeCommand(
        &node,
        QStringLiteral("Edited port '%1' of node '%2'")
            .arg(toString(srcPort), relativeNodePath(node)));
    Q_UNUSED(cmd);

    auto port = srcPort;
    port.typeId = dialog.typeId();
    port.caption = dialog.caption();
    port.captionVisible = dialog.captionVisible();
    port.optional = dialog.optional();
    srcPort.assign(port);
    assert(srcPort.id() == srcPort.id());
    emit node.portChanged(srcPort.id());
}

} // namespace

void
NodeUI::addDynamicInPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    addPort(*node, PortType::In);
}

void
NodeUI::addDynamicOutPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    addPort(*node, PortType::Out);
}

void
NodeUI::editDynamicPort(Node* node, PortType type, PortIndex idx)
{
    assert(node);

    PortId srcPortId = node->portId(type, idx);
    NodePort* srcPort = node->port(srcPortId);
    if(!srcPort) return;

    DynamicNode* dynNode = toDynamicNode(node);
    if (!dynNode) return;

    editPort(*dynNode, type, *srcPort);
}

void
NodeUI::deleteDynamicPort(Node* obj, PortType type, PortIndex idx)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    PortId portId = node->portId(type, idx);
    if (!portId.isValid()) return;

    Graph* graph = Graph::accessGraph(*node);
    assert(graph);
    Node::PortInfo* port = node->port(portId);
    assert(port);

    auto cmd = gtApp->makeCommand(graph->rootGraph(),
                                  QStringLiteral("Deleting port '%1' of node '%2'")
                                      .arg(toString(*port), relativeNodePath(*node)));
    Q_UNUSED(cmd);

    node->removePort(portId);
}


void
NodeUI::editUserVariables(GtObject* obj)
{
    Graph* graph = toGraph(obj);
    if (!graph)
    {
        GraphUserVariablesInputNode* node = toUserVariablesNode(obj);
        if (!node) return;

        graph = Graph::accessGraph(*node);
        graph = graph->rootGraph();
        if (!graph) return;
    }
    if (!isRootGraph.get()(graph)) return;

    GraphUserVariablesDialog dialog{*graph};
    dialog.exec();
}

bool
NodeUI::deleteDummyNode(Node* node)
{
    DummyNode* dummy = toDummyNode(node);
    if (!dummy) return false;

    GtObject* linkedObject = dummy->linkedObject();
    if (!linkedObject) return false;

    assert(linkedObject->isDummy());

    auto result = QMessageBox::warning(
        nullptr,
        tr("Delete dummy object '%1'").arg(dummy->caption()),
        tr("Deleting the dummy node will also delete the\n"
           "corresponding dummy object in the data model.\n"
           "Do you want to proceed?"),
        QMessageBox::Cancel | QMessageBox::Yes,
        QMessageBox::Yes
    );

    if (result != QMessageBox::Yes) return false;

    auto cmd = gtApp->makeCommand(dummy->parentObject(),
                                  tr("Delete dummy object '%1'")
                                      .arg(dummy->caption()));
    Q_UNUSED(cmd);

    delete dummy;
    delete linkedObject;
    return true;
}
void
NodeUI::setActive(GtObject* obj, bool state)
{
    auto* node = toNode(obj);
    if (!node) return;

    auto cmd = gtApp->makeCommand(node, (state ?
                                             tr("Paused node '%1'") :
                                             tr("Unpaused node '%1"))
                                                .arg(relativeNodePath(*node)));
    Q_UNUSED(cmd);

    node->setActive(state);
}

QList<PortUIAction> const&
intelli::NodeUI::portActions() const
{
    return pimpl->portActions;
}
