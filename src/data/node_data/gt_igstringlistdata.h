/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGSTRINGLISTDATA_H
#define GT_IGSTRINGLISTDATA_H

#include "gt_ignodedata.h"

class GT_IG_EXPORT GtIgStringListData : public GtIgTemplateData<QStringList>
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgStringListData(QStringList list = {});
};

#endif // GT_IGSTRINGLISTDATA_H
