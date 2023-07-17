/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphpackageui.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphcategory.h"
#include "gt_igpackage.h"

#include "gt_icons.h"
#include "gt_regexp.h"
#include "gt_qtutilities.h"
#include "gt_inputdialog.h"
#include "gt_datamodel.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegExpValidator>
#include <QFileInfo>
#include <QFile>

inline void setObjectName(GtObject& obj, QString const& name)
{
    obj.setObjectName(name);
}
inline void setObjectName(GtIntelliGraphNode& obj, QString const& name)
{
    obj.setCaption(name);
}

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
    setObjectName(*child, gt::makeUniqueName(text, obj));

    if (gtDataModel->appendChild(child.get(), &obj).isValid())
    {
        child.release();
    }
}

GtIntelliGraphPackageUI::GtIntelliGraphPackageUI()
{
    setObjectName(QStringLiteral("IntelliGraphObjectUI"));

    addSingleAction(tr("Add Category"), addNodeCategory)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isPackageObject);

    addSingleAction(tr("Add Intelli Graph"), addNodeGraph)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isCategoryObject);
}

QIcon
GtIntelliGraphPackageUI::icon(GtObject* obj) const
{
    if (qobject_cast<GtIgPackage*>(obj))
    {
        return gt::gui::icon::applicationVar();
    }
    return gt::gui::icon::objectEmpty();
}

void
GtIntelliGraphPackageUI::addNodeCategory(GtObject* obj)
{
    if (!isPackageObject(obj)) return;

    addNamedChild<GtIntelliGraphCategory>(*obj);
}

void
GtIntelliGraphPackageUI::addNodeGraph(GtObject* obj)
{
    if (!isCategoryObject(obj)) return;

    addNamedChild<GtIntelliGraph>(*obj);
}

bool
GtIntelliGraphPackageUI::isCategoryObject(GtObject* obj)
{
    return qobject_cast<GtIntelliGraphCategory*>(obj);
}

bool
GtIntelliGraphPackageUI::isPackageObject(GtObject* obj)
{
    return qobject_cast<GtIgPackage*>(obj);
}
