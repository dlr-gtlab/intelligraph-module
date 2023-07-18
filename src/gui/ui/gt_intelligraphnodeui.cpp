/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphnodeui.h"

#include "gt_igjsonadpater.h"
#include "gt_intelligraph.h"
#include "gt_intelligrapheditor.h"

#include "gt_utilities.h"
#include "gt_logging.h"
#include "gt_icons.h"
#include "gt_inputdialog.h"
#include "gt_application.h"
#include "gt_command.h"
#include "gt_filedialog.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
#include <QFileInfo>
#include <QFile>

GtIntelliGraphNodeUI::GtIntelliGraphNodeUI()
{
    setObjectName(QStringLiteral("IntelliGraphNodeUI"));

    addSingleAction(tr("Rename Node"), renameNode)
        .setIcon(gt::gui::icon::rename())
        .setVisibilityMethod(isNodeObject)
        .setVerificationMethod(canRenameNodeObject);

    addSingleAction(tr("Clear Intelli Graph"), clearNodeGraph)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(isGraphObject);

    addSingleAction(tr("Load Intelli Graph..."), loadNodeGraph)
        .setIcon(gt::gui::icon::import())
        .setVisibilityMethod(isGraphObject);
}

QIcon
GtIntelliGraphNodeUI::icon(GtObject* obj) const
{
    if (qobject_cast<GtIntelliGraph*>(obj))
    {
        return gt::gui::icon::application();
    }

    return gt::gui::icon::objectEmpty();
}

void
GtIntelliGraphNodeUI::renameNode(GtObject* obj)
{
    auto* node = qobject_cast<GtIntelliGraphNode*>(obj);
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
    auto graph = qobject_cast<GtIntelliGraph*>(obj);

    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    graph->clear();
}

void
GtIntelliGraphNodeUI::loadNodeGraph(GtObject* obj)
{
    auto graph = qobject_cast<GtIntelliGraph*>(obj);

    if (!graph) return;

    QString filePath = GtFileDialog::getOpenFileName(nullptr, tr("Open Intelli Flow"));

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        gtError() << tr("Failed to open intelli flow from file! (%1)")
                         .arg(filePath);
        return;
    }

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Loading IntelliGraph (%1)")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    auto scene = QJsonDocument::fromJson(file.readAll()).object();
    gt::ig::fromJson(scene, *graph);
}

bool
GtIntelliGraphNodeUI::isGraphObject(GtObject* obj)
{
    return qobject_cast<GtIntelliGraph*>(obj);
}

bool
GtIntelliGraphNodeUI::isNodeObject(GtObject* obj)
{
    return qobject_cast<GtIntelliGraphNode*>(obj);
}

bool
GtIntelliGraphNodeUI::canRenameNodeObject(GtObject* obj)
{
    if (auto* node = qobject_cast<GtIntelliGraphNode*>(obj))
    {
        return !(node->nodeFlags() & gt::ig::Unique);
    }
    return true;
}

QStringList
GtIntelliGraphNodeUI::openWith(GtObject* obj)
{
    QStringList list;

    if (isGraphObject(obj))
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

