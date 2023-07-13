/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTIGBOOLDATA_H
#define GTIGBOOLDATA_H

#include "gt_ignodedata.h"

class GtIgBoolData : public GtIgTemplateData<bool>
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgBoolData(bool val = {});
};

#endif // GTIGBOOLDATA_H
