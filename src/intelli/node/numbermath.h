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

#include <QMetaType>

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

    MathOperation operation() const;
    void setOperation(MathOperation op);

signals:

    // Forwarded across threads (DetachedExecutor), so the enum must be a Qt metatype.
    void operationChanged(MathOperation op);

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

// Required for queued connections of NumberMathNode::operationChanged.
Q_DECLARE_METATYPE(intelli::NumberMathNode::MathOperation)

#endif // GT_INTELLI_NUMBERMATHNODE_H
