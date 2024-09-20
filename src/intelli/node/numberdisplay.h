/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NUMBERDISPLAYNODE_H
#define GT_INTELLI_NUMBERDISPLAYNODE_H

#include <intelli/node.h>

namespace intelli
{

class NumberDisplayNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE NumberDisplayNode();
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERDISPLAYNODE_H
