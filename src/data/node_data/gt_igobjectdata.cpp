/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igobjectdata.h"

#include "gt_intelligraphdatafactory.h"

GTIG_REGISTER_DATA(GtIgObjectData)

GtIgObjectData::GtIgObjectData(GtObject const* obj) :
    GtIgNodeData("object"),
    m_obj(std::unique_ptr<GtObject>(obj ? obj->clone() : nullptr))
{

}
