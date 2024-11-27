/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/ui/packageui.h"

#include "intelli/graphcategory.h"
#include "intelli/package.h"
#include "intelli/private/utils.h"

#include "gt_icons.h"

#include <QJsonObject>
#include <QJsonDocument>

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



PackageUI::PackageUI()
{
    setObjectName(QStringLiteral("IntelliGraphObjectUI"));

    addSingleAction(tr("Add Category"), addNodeCategory)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isPackageObject);
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
    
    utils::addNamedChild<GraphCategory>(*obj);
}

void
PackageUI::addNodeGraph(GtObject* obj)
{
    if (!isCategoryObject(obj)) return;
    
    utils::addNamedChild<Graph>(*obj);
}

bool
PackageUI::isCategoryObject(GtObject* obj)
{
    return false;
}

bool
PackageUI::isPackageObject(GtObject* obj)
{
    return qobject_cast<Package*>(obj);
}
