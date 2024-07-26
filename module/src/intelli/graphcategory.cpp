/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/graphcategory.h"

using namespace intelli;

GraphCategory::GraphCategory()
{
    setObjectName(tr("Category"));

    setFlag(UserRenamable, true);
    setFlag(UserDeletable, true);
}
