/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/connectiongroup.h"

#include "gt_coreapplication.h"

using namespace intelli;

ConnectionGroup::ConnectionGroup(GtObject* parent) :
    GtObjectGroup(parent)
{
    setObjectName(QStringLiteral("__connections"));

    if (!gtApp || !gtApp->devMode()) setFlag(UserHidden);
}

void
ConnectionGroup::onObjectDataMerged()
{
    // the user may have deleted connections only, which must be restored
    emit mergeConnections();
}
