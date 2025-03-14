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
#include "intelli/node/dummy.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/icons.h"
#include "intelli/gui/nodeuidata.h"
#include "intelli/gui/nodegeometry.h"
#include "intelli/gui/nodepainter.h"
#include "intelli/gui/graphics/nodeobject.h"
#include "intelli/private/utils.h"

#include <gt_logging.h>

#include <gt_command.h>
#include <gt_inputdialog.h>
#include <gt_application.h>

#include <QFileInfo>
#include <QFile>


using BoolObjectMethod = std::function<bool (GtObject*)>;

template <typename Functor>
inline BoolObjectMethod NOT(Functor fA)
{
    return [a = std::move(fA)](GtObject* obj){
        return !a(obj);
    };
}
template <typename Functor>
inline BoolObjectMethod operator+(BoolObjectMethod fA, Functor fOther)
{
    return [a = std::move(fA), b = std::move(fOther)](GtObject* obj){
        return a(obj) && b(obj);
    };
}

using namespace intelli;

struct NodeUI::Impl
{
    /// List of custom port actions
    QList<PortUIAction> portActions;
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
        .setVisibilityMethod(toNode);

    addSingleAction(tr("Set inactive"), setActive<false>)
        .setIcon(gt::gui::icon::sleep())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("skipProcess"), categroy))
        .setVisibilityMethod(toNode + isActive);

    addSingleAction(tr("set active"), setActive<true>)
        .setIcon(gt::gui::icon::sleepOff())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("unskipProcess"), categroy))
        .setVisibilityMethod(toNode + NOT(isActive));

    addSeparator();

#if GT_VERSION >= 0x020100
    setRegExpHint(tr("It is only allowed to use letters, numbers, '_', '-' "
                     "and '[ ]' to rename the object and is not allowed to "
                     "use the name of sibling elements."));
#endif
    addSingleAction(tr("Rename Node"), renameNode)
        .setIcon(gt::gui::icon::rename())
        .setVisibilityMethod(toNode)
        .setVerificationMethod(canRenameNodeObject)
        .setShortCut(gtApp->getShortCutSequence("rename"));

    addSingleAction(tr("Clear Intelli Graph"), clearNodeGraph)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(toGraph);

    if ((option & NoDefaultPortActions)) return;

    auto const hasOutputPorts = [](GtObject* obj){
        return !(static_cast<DynamicNode*>(obj)->dynamicNodeOption() &
                 DynamicNode::DynamicInputOnly);
    };
    auto const hasInputPorts = [](GtObject* obj){
        return !(static_cast<DynamicNode*>(obj)->dynamicNodeOption() &
                 DynamicNode::DynamicOutputOnly);
    };

    addSeparator();

    addSingleAction(tr("Add In Port"), addInPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toDynamicNode + hasInputPorts);

    addSingleAction(tr("Add Out Port"), addOutPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toDynamicNode + hasOutputPorts);

    /** PORT ACTIONS **/

    addPortAction(tr("Delete Port"), deleteDynamicPort)
        .setIcon(gt::gui::icon::delete_())
        .setVerificationMethod(isDynamicPort)
        .setVisibilityMethod(isDynamicNode);

    if (!gtApp || !gtApp->devMode()) return;

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
    return uiData;
}

QIcon
NodeUI::icon(GtObject* obj) const
{
    Node* node = toNode(obj);
    if (!node) return gt::gui::icon::objectEmpty();

    QIcon icon = displayIcon(*node);
    if (!icon.isNull()) return icon;

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
        return gt::gui::icon::objectInvalid();
    }
    return QIcon{};
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
NodeUI::isDynamicPort(GtObject* obj, PortType type, PortIndex idx)
{
    if (auto* node = toDynamicNode(obj))
    {
        return node->isDynamicPort(type, idx);
    }
    return false;
}

bool
NodeUI::isDynamicNode(GtObject* obj, PortType, PortIndex)
{
    return toDynamicNode(obj);
}

bool
NodeUI::canRenameNodeObject(GtObject* obj)
{
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
    if (!node) return;

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

void
NodeUI::addInPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addInPort(typeId<DoubleData>());
    gtInfo().verbose() << tr("Added dynamic in port with id") << id;
}

void
NodeUI::addOutPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addOutPort(typeId<DoubleData>());
    gtInfo().verbose() << tr("Added dynamic out port with id") << id;
}

void
NodeUI::deleteDynamicPort(Node* obj, PortType type, PortIndex idx)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    node->removePort(node->portId(type, idx));
}

void
NodeUI::clearNodeGraph(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });
    
    graph->clearGraph();
}

void
NodeUI::setActive(GtObject* obj, bool state)
{
    auto* node = toNode(obj);
    if (!node) return;

    node->setActive(state);
}

#if GT_VERSION >= 0x020100
bool
NodeUI::hasValidationRegExp(GtObject* obj)
{
    return true;
}

QRegExp
NodeUI::validatorRegExp(GtObject* obj)
{
    assert(obj);

    QRegExp retVal = gt::re::onlyLettersAndNumbersAndSpace();

    if (toGraph(obj))
    {
        utils::restrictRegExpWithSiblingsNames<Graph>(*obj, retVal);
    }
    else
    {
        utils::restrictRegExpWithSiblingsNames<Node>(*obj, retVal);
    }

    return retVal;
}
#endif

QList<PortUIAction> const&
intelli::NodeUI::portActions() const
{
    return pimpl->portActions;
}
