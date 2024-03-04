/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_LOGICSOURCENODE_H
#define GT_INTELLI_LOGICSOURCENODE_H

#include <intelli/node.h>

#include <gt_boolproperty.h>

namespace intelli
{

class LogicSourceNode : public intelli::Node
{
    Q_OBJECT

public:
    
    Q_INVOKABLE LogicSourceNode();

protected:

    void eval() override;

private:

    PortId m_out;

    GtBoolProperty m_value;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICSOURCENODE_H
