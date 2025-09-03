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
#include "intelli/private/gui_utils.h"

#include <gt_icons.h>

using namespace intelli;


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
    return gt::gui::icon::applicationVar();
}

void
PackageUI::addNodeCategory(GtObject* obj)
{
    gui_utils::addNamedChild<GraphCategory>(*obj);
}

bool
PackageUI::isPackageObject(GtObject* obj)
{
    return qobject_cast<Package*>(obj);
}
