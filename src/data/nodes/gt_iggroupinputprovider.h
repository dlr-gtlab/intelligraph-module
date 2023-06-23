/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGGROUPINPUTPROVIDER_H
#define GT_IGGROUPINPUTPROVIDER_H

#include "gt_intelligraphnode.h"

class GtIgGroupInputProvider : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgGroupInputProvider();

protected:

    NodeData eval(PortId outId) override;
};

#endif // GT_IGGROUPINPUTPROVIDER_H
