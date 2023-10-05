/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NUMBERINPUTNODE_H
#define GT_INTELLI_NUMBERINPUTNODE_H

#include <intelli/node.h>

#include <gt_doubleproperty.h>

namespace intelli
{

class NumberSourceNode : public Node
{
    Q_OBJECT

public:
    
    Q_INVOKABLE NumberSourceNode();

protected:

    void eval() override;

private:

    GtDoubleProperty m_value;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERINPUTNODE_H
