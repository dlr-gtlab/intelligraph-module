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
#include "intelli/graphutilities.h"
#include "intelli/node/dummy.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/gui/commentgroup.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/guidata.h"
#include "intelli/gui/icons.h"
#include "intelli/gui/nodeuidata.h"
#include "intelli/gui/nodegeometry.h"
#include "intelli/gui/nodepainter.h"
#include "intelli/gui/graphics/nodeobject.h"
#include "intelli/gui/widgets/graphuservariablesdialog.h"
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

using BoolObjectMethod = std::function<bool (GtObject*)>;
using BoolPortMethod = std::function<bool (Node*, PortType, PortIndex)>;

using DeleteAction = std::pair<NodeUI::CustomDeleteFunctor,
                               NodeUI::EnableCustomDeleteFunctor>;

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

struct NodeUI::Impl
{
    /// List of custom port actions
    QList<PortUIAction> portActions;

    QList<DeleteAction> deleteActions;
};

NodeUI::NodeUI(Option option) :
    pimpl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("IntelliGraphNodeUI"));

    if ((option & NoDefaultActions)) return;

    auto const isActive = [](GtObject* obj){
        return static_cast<Node*>(obj)->isActive();
    };

    static auto const& categroy =  QStringLiteral("GtProcessDock");

    addSingleAction(tr("Execute once"), executeNode)
        .setIcon(gt::gui::icon::processRun())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("runProcess"), categroy))
        .setVisibilityMethod(toNode * NOT(toDummy));

    addSingleAction(tr("Set inactive"), setActive<false>)
        .setIcon(gt::gui::icon::sleep())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("skipProcess"), categroy))
        .setVisibilityMethod(toNode * NOT(toDummy) * isActive);

    addSingleAction(tr("set active"), setActive<true>)
        .setIcon(gt::gui::icon::sleepOff())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("unskipProcess"), categroy))
        .setVisibilityMethod(toNode * NOT(toDummy) * NOT(isActive));

    addSeparator();

    addSingleAction(tr("Rename"), renameNode)
        .setIcon(gt::gui::icon::rename())
        .setVisibilityMethod(toNode)
        .setVerificationMethod(canRenameNodeObject)
        .setShortCut(gtApp->getShortCutSequence("rename"));

    addSeparator();

    addSingleAction(tr("Clear Graph"), clearGraphNode)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(toGraph);

    addSingleAction(tr("Duplicate Graph"), duplicateGraph)
        .setIcon(gt::gui::icon::duplicate())
        .setVisibilityMethod(toGraph)
        .setShortCut(gtApp->getShortCutSequence("clone"));

    addSingleAction(tr("Edit User Variables..."), editUserVariables)
        .setIcon(gt::gui::icon::variable())
        .setVisibilityMethod(isRootGraph);

    addSeparator();

    if (!(option & NoDefaultPortActions))
    {
        auto const hasInputPorts = [](GtObject* obj, auto ...){
            return  (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicInput);
        };
        auto const hasOutputPorts = [](GtObject* obj, auto ...){
            return  (static_cast<DynamicNode*>(obj)->dynamicNodeOption() & DynamicNode::DynamicOutput);
        };

        addSingleAction(tr("Add In Port"), addInPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toDynamicNode * NOT(toDummy) * hasInputPorts);

        addSingleAction(tr("Add Out Port"), addOutPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toDynamicNode * NOT(toDummy) * hasOutputPorts);

        /** PORT ACTIONS **/

        addPortAction(tr("Delete Port"), deleteDynamicPort)
            .setIcon(gt::gui::icon::delete_())
            .setVerificationMethod(BoolPortMethod{isDynamicPort} * isInputPort * hasInputPorts)
            .setVisibilityMethod(isDynamicNode * hasInputPorts);

        addPortAction(tr("Delete Port"), deleteDynamicPort)
            .setIcon(gt::gui::icon::delete_())
            .setVerificationMethod(BoolPortMethod{isDynamicPort} * isOutputPort * hasOutputPorts)
            .setVisibilityMethod(isDynamicNode * hasOutputPorts);
    }

    if (gtApp && gtApp->devMode())
    {
        addPortAction(tr("Port Info"), [](Node* obj, PortType type, PortIndex idx){
            if (!obj) return;
            gtInfo() << tr("Node '%1' (id: %2), Port id: %3")
                            .arg(obj->caption())
                            .arg(obj->id())
                            .arg(toString(obj->portId(type, idx)));
        }).setIcon(gt::gui::icon::bug());

        addSingleAction(tr("Refresh Node"), [](GtObject* obj){
            if (auto* node = toNode(obj)) emit node->nodeChanged();
        }).setIcon(gt::gui::icon::reload())
          .setVisibilityMethod(toNode);

        addSingleAction(tr("Print Debug Information"), [](GtObject* obj){
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
    }

    addSeparator();

    addCustomDeleteAction(tr("Delete Dummy Node"), deleteDummyNode, isDummy);
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
    if (toConstGraph(&node))
    {
        return gt::gui::icon::intelli::intelliGraph();
    }
    if (qobject_cast<GroupInputProvider const*>(&node))
    {
        return gt::gui::icon::import();
    }
    if (qobject_cast<GroupOutputProvider const*>(&node))
    {
        return gt::gui::icon::export_();
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
    QStringList list;

    if (toGraph(obj))
    {
        list << GT_CLASSNAME(GraphEditor);
    }

    return list;
}

PortUIAction&
NodeUI::addPortAction(const QString& actionText,
                                    PortActionFunction actionMethod)
{
    pimpl->portActions.append(PortUIAction(actionText, std::move(actionMethod)));
    return pimpl->portActions.back();
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

    auto model = GraphExecutionModel::accessExecModel(*graph);
    if (!model) return;

    auto const& nodeUuid = node->uuid();
    model->invalidateNode(nodeUuid);
    model->evaluateNode(nodeUuid).detach();
}

namespace
{

void
addPort(DynamicNode& node, PortType type)
{
    Graph* graph = Graph::accessGraph(node);
    assert(graph);

    auto cmd = gtApp->makeCommand(graph,
                                  QStringLiteral("Adding an %1put port to node '%2'")
                                      .arg(type == PortType::In ? "in" : "out",
                                           relativeNodePath(node)));
    Q_UNUSED(cmd);

    // TODO: add option for type id

    auto id = (type == PortType::In) ?
                  node.addInPort(typeId<DoubleData>()) :
                  node.addOutPort(typeId<DoubleData>());

    gtInfo().verbose() << QObject::tr("Added dynamic port '%1'")
                              .arg(toString(*node.port(id)));
}

} // namespace

void
NodeUI::addInPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    addPort(*node, PortType::In);
}

void
NodeUI::addOutPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    addPort(*node, PortType::Out);
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
    if (!isRootGraph(graph)) return;

    GraphUserVariablesDialog dialog{*graph};
    dialog.exec();
}

void
NodeUI::clearGraphNode(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->makeCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    Q_UNUSED(cmd);
    
    graph->clearGraph();
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
NodeUI::duplicateGraph(GtObject* obj)
{
    Graph* graph = toGraph(obj);
    if (!graph) return;

    GtObject* parent = graph->parentObject();

    auto cmd = gtApp->makeCommand(parent,
                                  tr("Duplicate graph '%1'")
                                      .arg(relativeNodePath(*graph)));
    Q_UNUSED(cmd);

    utils::duplicateGraph(*graph);
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
