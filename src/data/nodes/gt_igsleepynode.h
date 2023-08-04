/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGSLEEPYNODE_H
#define GT_IGSLEEPYNODE_H

#include "gt_intelligraphnode.h"

class GtIgSleepyNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgSleepyNode();

protected:

    NodeData eval(PortId outId) override;

private:

    GtIntProperty m_timer;

    PortId m_in, m_out;
};

#endif // GT_IGSLEEPYNODE_H
