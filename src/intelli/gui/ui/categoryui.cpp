/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "intelli/gui/ui/categoryui.h"

#include "intelli/graph.h"
#include "intelli/graphcategory.h"
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



CategoryUI::CategoryUI()
{
    setObjectName(QStringLiteral("IntelliGraphCategoryUI"));

    addSingleAction(tr("Add Intelli Graph"), addNodeGraph)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(isCategoryObject);
}

QIcon
CategoryUI::icon(GtObject* obj) const
{
    return gt::gui::icon::objectEmpty();
}

void
CategoryUI::addNodeGraph(GtObject* obj)
{
    utils::addNamedChild<Graph>(*obj);
}

bool
CategoryUI::isCategoryObject(GtObject* obj)
{
    return qobject_cast<GraphCategory*>(obj);
}
