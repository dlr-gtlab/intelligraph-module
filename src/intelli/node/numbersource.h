/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
