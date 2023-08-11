/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONNECTIONGROUP_H
#define GT_INTELLI_CONNECTIONGROUP_H

#include "gt_objectgroup.h"

namespace intelli
{

class ConnectionGroup : public GtObjectGroup
{
    Q_OBJECT

public:

    Q_INVOKABLE ConnectionGroup(GtObject* parent = nullptr);

signals:

    void mergeConnections();

protected:

    // keep graph model up date if a connection was restored
    void onObjectDataMerged() override;
};

} // namespace intelli

#endif // GT_INTELLI_CONNECTIONGROUP_H
