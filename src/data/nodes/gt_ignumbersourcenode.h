/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGNUMBERINPUTNODE_H
#define GT_IGNUMBERINPUTNODE_H

#include "gt_intelligraphnode.h"
#include "gt_doubleproperty.h"

class GtIgNumberSourceNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:
    
    Q_INVOKABLE GtIgNumberSourceNode();

protected:

    NodeData eval(PortId outId) override;

private:

    GtDoubleProperty m_value;

    PortId m_out;
};

#endif // GT_IGNUMBERINPUTNODE_H
