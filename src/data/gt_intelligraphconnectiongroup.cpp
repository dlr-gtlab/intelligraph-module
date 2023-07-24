/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphconnectiongroup.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphmodeladapter.h"

#include "gt_coreapplication.h"

GtIntellIGraphConnectionGroup::GtIntellIGraphConnectionGroup(GtObject* parent) :
    GtObjectGroup(parent)
{
    setObjectName(QStringLiteral("__connections"));

    if (!gtApp || !gtApp->devMode()) setFlag(UserHidden);
}

void
GtIntellIGraphConnectionGroup::onObjectDataMerged()
{
    // the user may have deleted connections only, which must be restored
    auto* ig = qobject_cast<GtIntelliGraph*>(parent());
    if (!ig) return;

    if (auto* adapter = ig->findModelAdapter())
    {
        adapter->mergeConnections(*ig);
    }
}
