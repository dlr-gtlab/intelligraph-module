/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CHECKDOUBLENODE_H
#define GT_INTELLI_CHECKDOUBLENODE_H

#include <intelli/node.h>

namespace intelli
{

class CheckDoubleNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE CheckDoubleNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    PortId m_inA, m_inB, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_CHECKDOUBLENODE_H
