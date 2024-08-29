/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONNECTIONUI_H
#define GT_INTELLI_CONNECTIONUI_H

#include "gt_objectui.h"

namespace intelli
{

class ConnectionUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE ConnectionUI();

    QIcon icon(GtObject* obj) const override;
};

} // namespace intelli

#endif // GT_INTELLI_CONNECTIONUI_H
