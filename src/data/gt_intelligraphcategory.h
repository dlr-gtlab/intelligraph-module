/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GTINTELLIGRAPHCATEGORY_H
#define GTINTELLIGRAPHCATEGORY_H

#include "gt_object.h"
#include "gt_intelligraph_exports.h"

class GtIntelliGraphCategory : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIntelliGraphCategory();
};

#endif // GTINTELLIGRAPHCATEGORY_H
