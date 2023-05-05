/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphcategory.h"

GtIntelliGraphCategory::GtIntelliGraphCategory()
{
    setObjectName(tr("Category"));

    setFlag(UserRenamable, true);
    setFlag(UserDeletable, true);
}
