/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igbooldata.h"

#include "gt_intelligraphdatafactory.h"

GTIG_REGISTER_DATA(GtIgBoolData)

GtIgBoolData::GtIgBoolData(bool val) :
    GtIgTemplateData("boolean", val)
{

}

