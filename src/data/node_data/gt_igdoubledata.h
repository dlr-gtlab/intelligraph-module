/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GTIGDOUBLEDATA_H
#define GTIGDOUBLEDATA_H

#include "gt_ignodedata.h"

class GT_IG_EXPORT GtIgDoubleData : public GtIgTemplateData<double>
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgDoubleData(double val = {});
};

#endif // GTIGDOUBLEDATA_H
