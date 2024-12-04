/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "intelli/gui/ui/graphcategoryui.h"

#include "intelli/graph.h"
#include "intelli/graphcategory.h"
#include "intelli/private/utils.h"
#include "gt_logging.h"

#include "gt_icons.h"

#include <QJsonObject>
#include <QJsonDocument>

#include <QFileInfo>
#include <QFile>

using namespace intelli;

GraphCategoryUI::GraphCategoryUI()
{
    setObjectName(QStringLiteral("IntelliGraphCategoryUI"));

    addSingleAction(tr("Add Intelli Graph"), addNodeGraph)
        .setIcon(gt::gui::icon::add());

#if GT_VERSION >= 0x020100
    setRegExpHint(tr("It is only allowed to use letters, numbers, '_', '-' "
                     "and '[ ]' to rename the object and is not allowed to "
                     "use the name of another category"));
#endif
}

QIcon
GraphCategoryUI::icon(GtObject* obj) const
{
    return gt::gui::icon::objectEmpty();
}

void
GraphCategoryUI::addNodeGraph(GtObject* obj)
{
    utils::addNamedChild<Graph>(*obj);
}

#if GT_VERSION >= 0x020100
bool
GraphCategoryUI::hasValidationRegExp(GtObject* obj)
{
    return true;
}

QRegExp
GraphCategoryUI::validatorRegExp(GtObject* obj)
{
    assert(obj);

    QRegExp retVal = gt::re::onlyLettersAndNumbersAndSpace();

    utils::restrictRegExpWithSiblingsNames<GraphCategory>(*obj, retVal);

    return retVal;
}
#endif
