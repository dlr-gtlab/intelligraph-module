/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SLEEPYNODE_H
#define GT_INTELLI_SLEEPYNODE_H

#include <intelli/node.h>

#include <gt_intproperty.h>

namespace intelli
{

class SleepyNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE SleepyNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    GtIntProperty m_timer;

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_SLEEPYNODE_H