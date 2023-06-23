/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igdoubledata.h"

#include "gt_intelligraphdatafactory.h"

GTIG_REGISTER_DATA(GtIgDoubleData)

GtIgDoubleData::GtIgDoubleData(double val) :
    GtIgTemplateData("double", val)
{

}
