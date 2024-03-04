/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef LOGICNODE_H
#define LOGICNODE_H

#include <intelli/dynamicnode.h>

#include <gt_enumproperty.h>

namespace intelli
{

class LogicNode : public intelli::Node
{
    Q_OBJECT

public:

    enum LogicOperation
    {
        NOT,
        AND,
        OR,
        XOR,
        NAND,
        NOR,
    };
    Q_ENUM(LogicOperation)

    Q_INVOKABLE LogicNode();

protected:

    void eval() override;

private:

    PortId m_inA, m_inB, m_out;

    GtEnumProperty<LogicOperation> m_operation;

    QString toString(LogicOperation op) const;

    LogicOperation toLogicOperation(QString const& op) const;
};

} // namespace intelli

#endif // LOGICNODE_H
