/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_BOOLDATA_H
#define GT_INTELLI_BOOLDATA_H

#include "intelli/nodedata.h"

namespace intelli
{

class BoolData : public TemplateData<bool>
{
    Q_OBJECT

public:

    Q_INVOKABLE BoolData(bool val = {});
};

} // namespace intelli

using GtIgBoolData [[deprecated]] = intelli::BoolData;

#endif // GT_INTELLI_BOOLDATA_H
