/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igstringlistdata.h"

#include "gt_intelligraphdatafactory.h"

GTIG_REGISTER_DATA(GtIgStringListData)

GtIgStringListData::GtIgStringListData(QStringList list) :
    GtIgTemplateData("stringlist", std::move(list))
{

}
