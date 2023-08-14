/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/nodeui.h"

#include "intelli/data/double.h"
#include "intelli/dynamicnode.h"
#include "intelli/adapter/jsonadapter.h"
#include "intelli/graph.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/node.h"
#include "intelli/gui/icons.h"

#include "gt_logging.h"

#include "gt_command.h"
#include "gt_utilities.h"
#include "gt_inputdialog.h"
#include "gt_application.h"
#include "gt_filedialog.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
#include <QFileInfo>
#include <QFile>

using namespace intelli;

NodeUI::NodeUI(Option option)
{
    setObjectName(QStringLiteral("IntelliGraphNodeUI"));

    static auto const LOGICAL_AND = [](auto fA, auto fB){
        return [a = std::move(fA), b = std::move(fB)](GtObject* obj){
            return a(obj) && b(obj);
        };
    };
    static auto const LOGICAL_NOT = [](auto fA){
        return [a = std::move(fA)](GtObject* obj){
            return !a(obj);
        };
    };

    auto const isActive = [](GtObject* obj){
        return static_cast<Node*>(obj)->isActive();
    };
    auto const isEvaluationEnabled = [](GtObject* obj){
        return !(static_cast<Node*>(obj)->nodeFlags() & DoNotEvaluate);
    };

    addSingleAction(tr("Execute once"), executeOnce)
        .setIcon(gt::gui::icon::processRun())
        .setVisibilityMethod(LOGICAL_AND(LOGICAL_AND(toNode, LOGICAL_NOT(toGraph)),
                                         isEvaluationEnabled));

    addSingleAction(tr("Set inactive"), toggleActive)
        .setIcon(gt::gui::icon::sleep())
        .setVisibilityMethod(LOGICAL_AND(LOGICAL_AND(toNode, LOGICAL_NOT(toGraph)),
                                         LOGICAL_AND(isEvaluationEnabled, isActive)));

    addSingleAction(tr("set active"), toggleActive)
        .setIcon(gt::gui::icon::sleepOff())
        .setVisibilityMethod(LOGICAL_AND(LOGICAL_AND(toNode, LOGICAL_NOT(toGraph)),
                                         LOGICAL_AND(isEvaluationEnabled, LOGICAL_NOT(isActive))));

    addSeparator();

    addSingleAction(tr("Rename Node"), renameNode)
        .setIcon(gt::gui::icon::rename())
        .setVisibilityMethod(toNode)
        .setVerificationMethod(canRenameNodeObject)
        .setShortCut(gtApp->getShortCutSequence("rename"));

    addSingleAction(tr("Clear Intelli Graph"), clearNodeGraph)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(toGraph);

    addSingleAction(tr("Load Intelli Graph..."), loadNodeGraph)
        .setIcon(gt::gui::icon::import())
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
        .setVisibilityMethod(LOGICAL_AND(toDynamicNode, hasInputPorts));

    addSingleAction(tr("Add Out Port"), addOutPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(LOGICAL_AND(toDynamicNode, hasOutputPorts));

    /** PORT ACTIONS **/

    addPortAction(tr("Delete Port"), deleteDynamicPort)
        .setIcon(gt::gui::icon::delete_())
        .setVerificationMethod(isDynamicPort)
        .setVisibilityMethod(isDynamicNode);
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

DynamicNode*
NodeUI::toDynamicNode(GtObject* obj)
{
    return qobject_cast<DynamicNode*>(obj);;
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
NodeUI::clearNodeGraph(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    graph->clear();
}

void
NodeUI::loadNodeGraph(GtObject* obj)
{
    auto graph = toGraph(obj);

    if (!graph) return;

    QString filePath = GtFileDialog::getOpenFileName(nullptr, tr("Open Intelli Flow"));

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        gtError() << tr("Failed to open intelli graph from file! (%1)")
                     .arg(filePath);
        return;
    }

    auto scene = QJsonDocument::fromJson(file.readAll()).object();
    auto restored = intelli::fromJson(scene);
    if (!restored)
    {
        gtError() << tr("Failed to restore intelli graph!");
        return;
    }

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Loading IntelliGraph (%1)")
                                          .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    graph->clear();
    graph->appendObjects(restored->nodes, restored->connections);
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
NodeUI::toggleActive(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node) return;

    node->setActive(!node->isActive());

    if (node->isActive()) node->updateNode();
}

void
NodeUI::executeOnce(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node) return;

    auto cleanup = gt::finally([node, old = node->isActive()](){
        node->setActive(old);
    });
    Q_UNUSED(cleanup);

    node->setActive();
    node->updateNode();
}
