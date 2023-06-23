/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGGROUPOUTPUTPROVIDER_H
#define GT_IGGROUPOUTPUTPROVIDER_H

#include "gt_intelligraphnode.h"

class GtIgGroupOutputProvider : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgGroupOutputProvider();

protected:

    NodeData eval(PortId outId) override;
};

#endif // GT_IGGROUPOUTPUTPROVIDER_H
