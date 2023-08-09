/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 10.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef NUMBERMATHNODE_H
#define NUMBERMATHNODE_H

#include "intelli/node.h"

#include <gt_enumproperty.h>

namespace intelli
{
namespace test
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
        Divide
    };
    Q_ENUM(MathOperation)

    Q_INVOKABLE NumberMathNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    PortId m_inA, m_inB, m_out;

    GtEnumProperty<MathOperation> m_operation;

    QString toString(MathOperation op) const;

    MathOperation toMathOperation(QString const& op) const;
};

} // namespace test

} // namespace intelli

#endif // NUMBERMATHNODE_H
