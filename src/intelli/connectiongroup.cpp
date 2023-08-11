/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/connectiongroup.h"

#include "intelli/graph.h"
#include "intelli/adapter/modeladapter.h"

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
