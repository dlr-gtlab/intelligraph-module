/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LOGICDISPLAYNODE_H
#define GT_INTELLI_LOGICDISPLAYNODE_H

#include <intelli/node.h>

namespace intelli
{

class LogicDisplayNode : public intelli::Node
{
    Q_OBJECT

public:

    Q_INVOKABLE LogicDisplayNode();

private:

    PortId m_in;
};

}

#endif // GT_INTELLI_LOGICDISPLAYNODE_H
