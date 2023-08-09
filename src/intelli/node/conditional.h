/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONDITIONALNODE_H
#define GT_INTELLI_CONDITIONALNODE_H

#include <intelli/node.h>
#include <intelli/property/stringselection.h>

namespace intelli
{

class ConditionalNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ConditionalNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    StringSelectionProperty m_dataType;

    PortId m_inCondition, m_inData, m_outIf, m_outElse;
};

} // namespace intelli

#endif // GT_INTELLI_CONDITIONALNODE_H
