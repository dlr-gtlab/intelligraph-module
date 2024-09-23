/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LOGICNODE_H
#define GT_INTELLI_LOGICNODE_H

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

    LogicOperation operation() const;

protected:

    void eval() override;

private:

    PortId m_inA, m_inB, m_out;

    GtEnumProperty<LogicOperation> m_operation;

    QString toString(LogicOperation op) const;

    LogicOperation toLogicOperation(QString const& op) const;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICNODE_H
