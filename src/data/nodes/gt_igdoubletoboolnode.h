/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTIGCHECKDOUBLENODE_H
#define GTIGCHECKDOUBLENODE_H

#include "gt_intelligraphnode.h"

class GtIgCheckDoubleNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgCheckDoubleNode();

protected:

    NodeData eval(PortId outId) override;

private:

    PortId m_inA, m_inB, m_out;
};

#endif // GTIGCHECKDOUBLENODE_H
