/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_CATEGORY_H
#define GT_INTELLI_CATEGORY_H

#include <intelli/exports.h>

#include <gt_object.h>

namespace intelli
{

class GraphCategory : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphCategory();
};

} // namespace intelli

#endif // GT_INTELLI_CATEGORY_H
