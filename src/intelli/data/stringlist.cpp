/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/stringlist.h"

#include "intelli/nodedatafactory.h"

using namespace intelli;

GT_INTELLI_REGISTER_DATA(StringListData)

StringListData::StringListData(QStringList list) :
    TemplateData("stringlist", std::move(list))
{

}
