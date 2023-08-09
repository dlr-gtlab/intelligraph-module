/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_STRINGLISTDATA_H
#define GT_INTELLI_STRINGLISTDATA_H

#include "intelli/nodedata.h"

namespace intelli
{

class GT_INTELLI_EXPORT StringListData : public TemplateData<QStringList>
{
    Q_OBJECT

public:

    Q_INVOKABLE StringListData(QStringList list = {});
};

} // namespace intelli

using GtIgStringListData [[deprecated]]  = intelli::StringListData;

#endif // GT_INTELLI_STRINGLISTDATA_H
