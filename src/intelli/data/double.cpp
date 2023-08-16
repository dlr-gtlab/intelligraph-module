/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/double.h"

#include "intelli/nodedatafactory.h"

using namespace intelli;

static auto init_once = [](){
    return GT_INTELLI_REGISTER_DATA(DoubleData)
}();

DoubleData::DoubleData(double val) :
    TemplateData("double", val)
{

}
