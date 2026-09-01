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
#include "intelli/graphexecutor.h"

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

using namespace intelli;

using DeleteAction = std::pair<NodeUI::CustomDeleteFunctor,
                               NodeUI::EnableCustomDeleteFunctor>;

using BoolObjectMethod = std::function<bool (GtObject*)>;
using BoolPortMethod = std::function<bool (Node*, PortType, PortIndex)>;

// TODO: expose as public API?
/// NOT operator
template <typename Functor>
inline BoolObjectMethod NOT(Functor fA)
{
    return [a = std::move(fA)](GtObject* obj){
        return !a(obj);
    };
}
/// AND operator
template <typename Functor>
inline BoolObjectMethod operator*(BoolObjectMethod fA, Functor fOther)
{
    return [a = std::move(fA), b = std::move(fOther)](GtObject* obj){
        return a(obj) && b(obj);
    };
}
template <typename Functor>
inline BoolPortMethod operator*(BoolPortMethod fA, Functor fOther)
{
    return [a = std::move(fA), b = std::move(fOther)](Node* obj, PortType type, PortIndex idx){
        return a(obj, type, idx) && b(obj, type, idx);
    };
}

DummyNode* toDummy(GtObject* obj) { return qobject_cast<DummyNode*>(obj); }

bool isDummy(Node const* obj) { return qobject_cast<DummyNode const*>(obj); }

GraphUserVariablesInputNode* toUserVariablesNode(GtObject* obj)
{
    return qobject_cast<GraphUserVariablesInputNode*>(obj);
}

struct NodeUI::Impl
{
    /// List of custom port actions
    QList<PortUIAction> portActions;

    QList<DeleteAction> deleteActions;
};

NodeUI::NodeUI(Options options) :
    pimpl(std::make_unique<Impl>())
{
    addCustomDeleteAction(tr("Delete Dummy Node"), deleteDummyNode, isDummy);

    if (!options.testFlag(Option::CustomNodeActionsOrder))
    {
        initializeNodeActions(NodeUI::defaultNodeActions());
    }
    if (!options.testFlag(Option::CustomPortActionsOrder))
    {
        initializePortActions(NodeUI::defaultPortActions());
    }
}

// allows to use variadic arguments
auto const hasInputPorts = [](GtObject* obj, auto ...){
    return  (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicInput);
};
auto const hasOutputPorts = [](GtObject* obj, auto ...){
    return  (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicOutput);
};

NodeUI::~NodeUI() = default;

NodeUI::ActionList<NodeUI::NodeAction, GtObjectUIAction>
NodeUI::defaultNodeActions() const
{
    auto const isActive = [](GtObject* obj){
        return static_cast<Node*>(obj)->isActive();
    };

    ActionList<NodeAction, GtObjectUIAction> actions;

    static auto const& category =  QStringLiteral("GtProcessDock");

    actions << ExecuteNodeAction <<
        makeSingleAction(tr("Execute once"), executeNode)
            .setIcon(gt::gui::icon::processRun())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("runProcess"), category))
            .setVisibilityMethod(toNode * NOT(toDummy));

    actions << SetActiveNodeAction <<
        makeSingleAction(tr("Set Inactive"), setActive<false>)
            .setIcon(gt::gui::icon::sleep())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("skipProcess"), category))
            .setVisibilityMethod(toNode * NOT(toDummy) * isActive);

    actions << SetActiveNodeAction <<
        makeSingleAction(tr("Set Active"), setActive<true>)
            .setIcon(gt::gui::icon::sleepOff())
            .setShortCut(gtApp->getShortCutSequence(QStringLiteral("unskipProcess"), category))
            .setVisibilityMethod(toNode * NOT(toDummy) * NOT(isActive));

    actions << makeSeparator();

    actions << RenameNodeAction <<
        makeSingleAction(tr("Rename"), renameNode)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(toNode)
            .setVerificationMethod(canRenameNodeObject)
            .setShortCut(gtApp->getShortCutSequence("rename"));

    actions << makeSeparator();

    actions << CustomNodeAction <<
        makeSingleAction(tr("Edit User Variables..."), editUserVariables)
            .setIcon(gt::gui::icon::variable())
            .setVisibilityMethod(toUserVariablesNode);

    actions << makeSeparator();

    actions << AddPortNodeAction <<
        makeSingleAction(tr("Add In Port"), addDynamicInPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toDynamicNode * NOT(toDummy) * hasInputPorts);

    actions << AddPortNodeAction <<
        makeSingleAction(tr("Add Out Port"), addDynamicOutPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toDynamicNode * NOT(toDummy) * hasOutputPorts);

    actions << makeSeparator();

    if (gtApp && gtApp->devMode())
    {
        actions << OtherNodeAction <<
            makeSingleAction(tr("Refresh Node"), [](GtObject* obj){
                if (auto* node = toNode(obj)) emit node->nodeChanged();
            }).setIcon(gt::gui::icon::reload())
                .setVisibilityMethod(toNode);

        actions << OtherNodeAction <<
            makeSingleAction(tr("Print Graph Debug Information"), [](GtObject* obj){
                if (auto* graph = toGraph(obj))
                {
                    QString const& path = relativeNodePath(*graph);
                    gtInfo().nospace() << "Local Connection Model: (" << path << ")";
                    debug(graph->connectionModel());
                    gtInfo().nospace() << "Global Connection Model: (" << path << ")";
                    debug(graph->globalConnectionModel());
                }
            }).setIcon(gt::gui::icon::bug())
              .setVisibilityMethod(toGraph);

        actions << OtherNodeAction <<
            makeSingleAction(tr("Print Debug Port Information"), [](GtObject* obj){
                if (auto* node = toNode(obj))
                {
                    QString const& path = relativeNodePath(*node);
                    gtInfo() << "### Node:" << path << node->uuid() << gt::brackets(toString(node->id()));
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
            }).setIcon(gt::gui::icon::bug())
                .setVisibilityMethod(toNode);

        actions << OtherNodeAction <<
            makeSingleAction(tr("Force Delete"), [](GtObject* obj){
                if (obj) obj->deleteLater();
            }).setIcon(gt::gui::icon::delete_());

        actions << makeSeparator();
    }

    return actions;
}

NodeUI::ActionList<NodeUI::PortAction, PortUIAction>
NodeUI::defaultPortActions() const
{
    ActionList<PortAction, PortUIAction> actions;

    static auto const& category =  QStringLiteral("GtProcessDock");

    actions << EditPortAction <<
        makePortAction(tr("Edit Port"), editDynamicPort)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(BoolPortMethod{isDynamicPort} * isInputPort * hasInputPorts);

    // for input ports
    actions << DeletePortAction <<
        makePortAction(tr("Delete Port"), deleteDynamicPort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(BoolPortMethod{isDynamicPort} * isInputPort * hasInputPorts);

    // for output ports
    actions << DeletePortAction <<
        makePortAction(tr("Delete Port"), deleteDynamicPort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(BoolPortMethod{isDynamicPort} * isOutputPort * hasOutputPorts);

    if (gtApp && gtApp->devMode())
    {
        actions << makePortSeparator();

        actions << OtherPortAction <<
            makePortAction(tr("Port Info"), [](Node* obj, PortType type, PortIndex idx){
                if (!obj) return;
                gtInfo() << tr("Node '%1' (id: %2), Port id: %3")
                                .arg(obj->caption())
                                .arg(obj->id())
                                .arg(toString(obj->portId(type, idx)));
            }).setIcon(gt::gui::icon::bug());

        actions << makePortAction("", nullptr);
    }

    return actions;
}

void
NodeUI::initializeNodeActions(NodeActionList const& actions)
{
    for (auto const& entry : actions)
    {
        addSingleAction(entry.action.text(), [act = entry.action.method()](GtObject* o){ act(nullptr, o); })
            .setIcon(entry.action.icon())
            .setVerificationMethod(entry.action.verificationMethod())
            .setVisibilityMethod(entry.action.visibilityMethod())
            .setShortCut(entry.action.shortCut());
    }
}

void
NodeUI::initializePortActions(PortActionList const& actions)
{
    std::transform(actions.begin(),
                   actions.end(),
                   std::back_inserter(pimpl->portActions),
                   [](auto const& entry){
        return entry.action;
    });
}

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

    if (toDummy(obj))
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

GtObjectUIAction
NodeUI::makeSeparator()
{
    return GtObjectUIAction{};
}

GtObjectUIAction
NodeUI::makeSingleAction(const QString& text, ActionFunction f)
{
    return GtObjectUIAction{text, std::move(f)};
}

PortUIAction&
NodeUI::addPortAction(QString const& actionText, PortActionFunction actionMethod)
{
    pimpl->portActions.append(PortUIAction(actionText, std::move(actionMethod)));
    return pimpl->portActions.back();
}

PortUIAction
NodeUI::makePortAction(QString const& actionText, PortActionFunction actionMethod)
{
    return PortUIAction(actionText, std::move(actionMethod));
}

PortUIAction
NodeUI::makePortSeparator()
{
    return PortUIAction{};
}

void
NodeUI::addCustomDeleteAction(QString const& text,
                              CustomDeleteFunctor deleteFunctor,
                              EnableCustomDeleteFunctor enableDeleteFunctor)
{
    pimpl->deleteActions.push_back({ deleteFunctor, enableDeleteFunctor });

    auto& action = addSingleAction(text, [f = std::move(deleteFunctor)](GtObject* obj) {
        f(qobject_cast<Node*>(obj));
    });
    action.setIcon(gt::gui::icon::delete_());
    action.setShortCut(gtApp->getShortCutSequence("delete"));
    action.setVisibilityMethod([f = std::move(enableDeleteFunctor)](GtObject* obj) {
        return f(qobject_cast<Node*>(obj));
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
    return qobject_cast<DynamicNode*>(obj);;
}

DynamicNode const*
NodeUI::toConstDynamicNode(GtObject const* obj)
{
    return qobject_cast<DynamicNode const*>(obj);;
}

bool
NodeUI::isRootGraph(GtObject const* obj)
{
    Graph const* graph = toConstGraph(obj);
    return graph && graph->rootGraph() == graph;
}

bool
NodeUI::isInputPort(Node* node, PortType type, PortIndex index)
{
    return node && type == PortType::In && node->ports(type).size() > index;
}

bool
NodeUI::isOutputPort(Node* node, PortType type, PortIndex index)
{
    return node && type == PortType::Out && node->ports(type).size() > index;
}

bool
NodeUI::isDynamicPort(Node* obj, PortType type, PortIndex idx)
{
    if (toDummy(obj)) return false;
    if (auto* node = toDynamicNode(obj))
    {
        return node->isDynamicPort(type, idx);
    }
    return false;
}

bool
NodeUI::isDynamicNode(Node* obj, PortType, PortIndex)
{
    return toDynamicNode(obj);
}

bool
NodeUI::canRenameNodeObject(GtObject* obj)
{
    if (!obj || toDummy(obj))
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

#if 1
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
    PortEditDialog dialog{type};
    if (!dialog.exec()) return;

    Node::PortInfo portInfo{dialog.typeId()};
    portInfo.caption = dialog.caption();
    portInfo.captionVisible = dialog.captionVisible();

    // TODO: undo/redo command not working, since multiple nodes are updated in parallel
    auto cmd = gtApp->makeCommand(&node,
                                  QStringLiteral("Adding an %1put port to conditional node '%2'")
                                      .arg(type == PortType::In ? "in" : "out",
                                           relativeNodePath(node)));
    Q_UNUSED(cmd);

    auto id = (type == PortType::In) ?
                  node.addInPort(std::move(portInfo)) :
                  node.addOutPort(std::move(portInfo));

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

    PortEditDialog dialog{type};
    dialog.setTypeId(srcPort->typeId);
    dialog.setCaption(srcPort->caption);
    dialog.setCaptionVisible(srcPort->captionVisible);
    if (!dialog.exec()) return;

    // TODO: undo/redo command not working, since multiple nodes are updated in parallel
    auto cmd = gtApp->makeCommand(
        node,
        QStringLiteral("Edited port '%1' of node '%2'")
            .arg(toString(*srcPort),
                 relativeNodePath(*node))
    );
    Q_UNUSED(cmd);

    auto port = *srcPort;
    port.typeId = dialog.typeId();
    port.caption = dialog.caption();
    port.captionVisible = dialog.captionVisible();
    srcPort->assign(port);
    assert(srcPort->id() == srcPortId);
    emit node->portChanged(srcPort->id());
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
    if (!isRootGraph(graph)) return;

    GraphUserVariablesDialog dialog{*graph};
    dialog.exec();
}

bool
NodeUI::deleteDummyNode(Node* node)
{
    DummyNode* dummy = toDummy(node);
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
