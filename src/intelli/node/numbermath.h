/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NUMBERMATHNODE_H
#define GT_INTELLI_NUMBERMATHNODE_H

#include "intelli/node.h"

#include <gt_enumproperty.h>

namespace intelli
{

class NumberMathNode : public Node
{
    Q_OBJECT

public:

    enum MathOperation
    {
        Plus,
        Minus,
        Multiply,
        Divide,
        Power
    };
    Q_ENUM(MathOperation)

    Q_INVOKABLE NumberMathNode();

protected:

    void eval() override;

private:

    PortId m_inA, m_inB, m_out;

    GtEnumProperty<MathOperation> m_operation;

    QString toString(MathOperation op) const;

    MathOperation toMathOperation(QString const& op) const;

    void updatePortCaptions();
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERMATHNODE_H
