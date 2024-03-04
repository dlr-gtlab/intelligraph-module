/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/nodeui.h"

#include "intelli/dynamicnode.h"
#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/icons.h"

#include <gt_logging.h>

#include <gt_command.h>
#include <gt_inputdialog.h>
#include <gt_application.h>

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
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

NodeUI::NodeUI(Option option)
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

    addSingleAction(tr("Node Info"), [](GtObject* obj){
        auto* node = toNode(obj);
        if (!node) return;
        auto* graph = obj->findParent<Graph*>();
        if (!graph) return;
        auto* model = graph->executionModel();
        if (!model) return;
        model->debug(node->id());
    }).setIcon(gt::gui::icon::bug());

    addSingleAction(tr("Update Node"), [](GtObject* obj){
        auto* node = toNode(obj);
        if (!node) return;
        emit node->nodeStateChanged();
    }).setIcon(gt::gui::icon::bug());

    addPortAction(tr("Port Info"), [](Node* obj, PortType type, PortIndex idx){
        if (!obj) return;
        gtInfo() << tr("Node '%1' (id: %2), Port id: %3")
                        .arg(obj->caption())
                        .arg(obj->id())
                        .arg(obj->portId(type, idx));
    }).setIcon(gt::gui::icon::bug());
}

QIcon
NodeUI::icon(GtObject* obj) const
{
    if (toGraph(obj))
    {
        return gt::gui::icon::intelli::intelliGraph();
    }

    return gt::gui::icon::intelli::node();
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
    m_portActions.append(PortUIAction(actionText, std::move(actionMethod)));
    return m_portActions.back();
}

Graph*
NodeUI::toGraph(GtObject* obj)
{
    return qobject_cast<Graph*>(obj);
}

Node*
NodeUI::toNode(GtObject* obj)
{
    return qobject_cast<Node*>(obj);
}

DynamicNode*
NodeUI::toDynamicNode(GtObject* obj)
{
    return qobject_cast<DynamicNode*>(obj);;
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

    auto cleanup = gt::finally([node, old = node->isActive()](){
        node->setActive(old);
    });
    Q_UNUSED(cleanup);
    
    auto* model = graph->makeExecutionModel();

    node->setActive();
    model->invalidateOutPorts(node->id());
    model->evaluateNode(node->id()).detach();
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

    auto wasActive = node->isActive();

    node->setActive(state);

    if (!wasActive && node->isActive())
    {
        emit node->triggerNodeEvaluation();
    }
}
