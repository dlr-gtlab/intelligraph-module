/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphnodeui.h"

#include "gt_igdoubledata.h"
#include "gt_intelligraphdynamicnode.h"
#include "gt_intelligraphjsonadapter.h"
#include "gt_intelligraph.h"
#include "gt_intelligrapheditor.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_igicons.h"

#include "gt_utilities.h"
#include "gt_logging.h"
#include "gt_inputdialog.h"
#include "gt_application.h"
#include "gt_command.h"
#include "gt_filedialog.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
#include <QFileInfo>
#include <QFile>

GtIntelliGraphNodeUI::GtIntelliGraphNodeUI(Option option)
{
    static auto const LOGICAL_AND = [](auto fA, auto fB){
        return [a = std::move(fA), b = std::move(fB)](GtObject* obj){
            return a(obj) && b(obj);
        };
    };

    setObjectName(QStringLiteral("IntelliGraphNodeUI"));

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
        return !(static_cast<GtIntelliGraphDynamicNode*>(obj)->dynamicNodeOption() &
                 GtIntelliGraphDynamicNode::DynamicInputOnly);
    };
    auto const hasInputPorts = [](GtObject* obj){
        return !(static_cast<GtIntelliGraphDynamicNode*>(obj)->dynamicNodeOption() &
                 GtIntelliGraphDynamicNode::DynamicOutputOnly);
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
        .setVisibilityMethod(isDynamicNode);
}

QIcon
GtIntelliGraphNodeUI::icon(GtObject* obj) const
{
    if (toGraph(obj))
    {
        return gt::gui::icon::ig::intelliGraph();
    }

    return gt::gui::icon::ig::node();
}

QStringList
GtIntelliGraphNodeUI::openWith(GtObject* obj)
{
    QStringList list;

    if (toGraph(obj))
    {
        list << GT_CLASSNAME(GtIntelliGraphEditor);
    }

    return list;
}

GtIgPortUIAction&
GtIntelliGraphNodeUI::addPortAction(const QString& actionText,
                                    PortActionFunction actionMethod)
{
    m_portActions.append(GtIgPortUIAction(actionText, std::move(actionMethod)));
    return m_portActions.back();
}

GtIntelliGraph*
GtIntelliGraphNodeUI::toGraph(GtObject* obj)
{
    return qobject_cast<GtIntelliGraph*>(obj);
}

GtIntelliGraphNode*
GtIntelliGraphNodeUI::toNode(GtObject* obj)
{
    return qobject_cast<GtIntelliGraphNode*>(obj);
}

GtIntelliGraphDynamicNode*
GtIntelliGraphNodeUI::isDynamicNode(GtObject* obj, PortType, PortIndex)
{
    return qobject_cast<GtIntelliGraphDynamicNode*>(obj);
}

GtIntelliGraphDynamicNode*
GtIntelliGraphNodeUI::toDynamicNode(GtObject* obj)
{
    return isDynamicNode(obj, {}, PortIndex{});
}

bool
GtIntelliGraphNodeUI::canRenameNodeObject(GtObject* obj)
{
    if (auto* node = toNode(obj))
    {
        return !(node->nodeFlags() & gt::ig::Unique);
    }
    return true;
}

void
GtIntelliGraphNodeUI::renameNode(GtObject* obj)
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
GtIntelliGraphNodeUI::clearNodeGraph(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    graph->clear();
}

void
GtIntelliGraphNodeUI::loadNodeGraph(GtObject* obj)
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
    auto restored = gt::ig::fromJson(scene);
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
GtIntelliGraphNodeUI::addInPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addInPort(gt::ig::typeId<GtIgDoubleData>());
    gtDebug() << id;
}

void
GtIntelliGraphNodeUI::addOutPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addOutPort(gt::ig::typeId<GtIgDoubleData>());
    gtDebug() << id;
}

void
GtIntelliGraphNodeUI::deleteDynamicPort(GtIntelliGraphNode* obj, PortType type, PortIndex idx)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    node->removePort(node->portId(type, idx));
}
