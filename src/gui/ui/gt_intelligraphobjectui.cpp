/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphobjectui.h"

#include "gt_intelligraph.h"
#include "gt_intelligrapheditor.h"
#include "gt_intelligraphcategory.h"
#include "gt_intelligraphnodegroup.h"
#include "gt_igpackage.h"
#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"
#include "gt_igdoubledata.h"

#include "gt_utilities.h"
#include "gt_logging.h"
#include "gt_icons.h"
#include "gt_regexp.h"
#include "gt_qtutilities.h"
#include "gt_inputdialog.h"
#include "gt_application.h"
#include "gt_datamodel.h"
#include "gt_command.h"
#include "gt_filedialog.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
#include <QFileInfo>
#include <QFile>

template <typename T>
inline void addNamedChild(GtObject& obj)
{
    GtInputDialog dialog{GtInputDialog::TextInput};

    dialog.setTextValidator(new QRegExpValidator{
        gt::re::onlyLettersAndNumbersAndSpace()
    });

    if (!dialog.exec()) return;

    QString text = dialog.textValue();
    if (text.isEmpty()) return;

    auto child = std::make_unique<T>();
    child->setObjectName(gt::makeUniqueName(text, obj));

    if (gtDataModel->appendChild(child.get(), &obj).isValid())
    {
        child.release();
    }
}

GtIntelliGraphObjectUI::GtIntelliGraphObjectUI()
{
    setObjectName(QStringLiteral("IntelliGraphObjectUI"));

    addSingleAction(tr("Add Category"), addNodeCategory)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isPackageObject);

    addSingleAction(tr("Add Intelli Graph"), addNodeGraph)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isCategoryObject);

    addSingleAction(tr("Add Node Group"), addNodeGroup)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isGraphObject);

    addSingleAction(tr("Clear Intelli Graph"), clearNodeGraph)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(isGraphObject);

    addSingleAction(tr("Load Intelli Graph..."), loadNodeGraph)
        .setIcon(gt::gui::icon::import())
        .setVisibilityMethod(isGraphObject);


    auto isProvider = [](GtObject* obj){
        return qobject_cast<GtIgGroupInputProvider*>(obj) ||
               qobject_cast<GtIgGroupOutputProvider*>(obj);
    };

    addSingleAction(tr("Insert port at front"), [=](GtObject* obj){
        if (!isProvider(obj)) return;

        GtInputDialog dialog;
        dialog.setInitialTextValue(GT_CLASSNAME(GtIgDoubleData));
        if (!dialog.exec()) return;

        if (auto* in = qobject_cast<GtIgGroupInputProvider*>(obj))
        {
            in->insertPort(dialog.textValue(), 0);
        }
        else if (auto* out = qobject_cast<GtIgGroupOutputProvider*>(obj))
        {
            out->insertPort(dialog.textValue(), 0);
        }
    })
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isProvider);

//    addSingleAction(tr("Remove Port at ..."), [](GtObject* obj){
//        auto* in = qobject_cast<GtIgGroupInputProvider*>(obj);
//        if (!in) return;

//        GtInputDialog dialog(GtInputDialog::IntInput);
//        dialog.setIntMaximum(in->ports(gt::ig::PortType::Out).size());
//        dialog.setIntMinimum(0);

//        if (!dialog.exec()) return;
//        in->removePort(gt::ig::PortIndex::fromValue(dialog.textValue().toInt()));
//    })
//        .setIcon(gt::gui::icon::remove())
//        .setVisibilityMethod(isInputProvider);
}

QIcon
GtIntelliGraphObjectUI::icon(GtObject* obj) const
{
    if (qobject_cast<GtIgPackage*>(obj))
    {
        return gt::gui::icon::applicationVar();
    }
    if (qobject_cast<GtIntelliGraph*>(obj))
    {
        return gt::gui::icon::application();
    }

    return gt::gui::icon::objectEmpty();
}

void
GtIntelliGraphObjectUI::addNodeCategory(GtObject* obj)
{
    if (!qobject_cast<GtIgPackage*>(obj)) return;

    addNamedChild<GtIntelliGraphCategory>(*obj);
}

void
GtIntelliGraphObjectUI::addNodeGraph(GtObject* obj)
{
    auto cat = qobject_cast<GtIntelliGraphCategory*>(obj);

    if (!cat) return;

    addNamedChild<GtIntelliGraph>(*cat);
}

void
GtIntelliGraphObjectUI::addNodeGroup(GtObject* obj)
{
    auto graph = qobject_cast<GtIntelliGraph*>(obj);

    if (!graph) return;

    graph->appendNode(std::make_unique<GtIntelliGraphNodeGroup>());
}

void
GtIntelliGraphObjectUI::clearNodeGraph(GtObject* obj)
{
    auto graph = qobject_cast<GtIntelliGraph*>(obj);

    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                   .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });

    graph->clear();
}

void
GtIntelliGraphObjectUI::loadNodeGraph(GtObject* obj)
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
    graph->fromJson(scene);
}

bool
GtIntelliGraphObjectUI::isCategoryObject(GtObject* obj)
{
    return qobject_cast<GtIntelliGraphCategory*>(obj);
}

bool
GtIntelliGraphObjectUI::isPackageObject(GtObject* obj)
{
    return qobject_cast<GtIgPackage*>(obj);
}

bool
GtIntelliGraphObjectUI::isGraphObject(GtObject* obj)
{
    return qobject_cast<GtIntelliGraph*>(obj);
}

QStringList
GtIntelliGraphObjectUI::openWith(GtObject* obj)
{
    QStringList list;

    if (isGraphObject(obj))
    {
        list << GT_CLASSNAME(GtIntelliGraphEditor);
    }

    return list;
}
