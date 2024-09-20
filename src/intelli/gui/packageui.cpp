/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/packageui.h"

#include "intelli/graph.h"
#include "intelli/graphcategory.h"
#include "intelli/package.h"

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

using namespace intelli;

inline void setObjectName(GtObject& obj, QString const& name)
{
    obj.setObjectName(name);
}
inline void setObjectName(Node& obj, QString const& name)
{
    obj.setCaption(name);
}

template <typename T>
inline void addNamedChild(GtObject& obj)
{
    GtInputDialog dialog{GtInputDialog::TextInput};
    dialog.setWindowTitle(QObject::tr("Name new Object"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(QObject::tr("Enter a name for the new object."));

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

PackageUI::PackageUI()
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
PackageUI::icon(GtObject* obj) const
{
    if (qobject_cast<Package*>(obj))
    {
        return gt::gui::icon::applicationVar();
    }
    return gt::gui::icon::objectEmpty();
}

void
PackageUI::addNodeCategory(GtObject* obj)
{
    if (!isPackageObject(obj)) return;
    
    addNamedChild<GraphCategory>(*obj);
}

void
PackageUI::addNodeGraph(GtObject* obj)
{
    if (!isCategoryObject(obj)) return;
    
    addNamedChild<Graph>(*obj);
}

bool
PackageUI::isCategoryObject(GtObject* obj)
{
    return qobject_cast<GraphCategory*>(obj);
}

bool
PackageUI::isPackageObject(GtObject* obj)
{
    return qobject_cast<Package*>(obj);
}
