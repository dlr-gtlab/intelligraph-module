/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHCONNECTIONGROUP_H
#define GTINTELLIGRAPHCONNECTIONGROUP_H

#include "gt_objectgroup.h"

class GtIntellIGraphConnectionGroup : public GtObjectGroup
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIntellIGraphConnectionGroup(GtObject* parent = nullptr);

protected:

    // keep graph model up date if a connection was restored
    void onObjectDataMerged() override;
};

#endif // GTINTELLIGRAPHCONNECTIONGROUP_H
