/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/bool.h"

#include "gt_logging.h"
#include "intelli/nodedatafactory.h"

using namespace intelli;

GTIG_REGISTER_DATA(BoolData)

BoolData::BoolData(bool val) :
    TemplateData("boolean", val)
{

}

