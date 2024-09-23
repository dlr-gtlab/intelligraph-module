/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
