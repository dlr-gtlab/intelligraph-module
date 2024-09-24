/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_STRINGBUILDERNODE_H
#define GT_INTELLI_STRINGBUILDERNODE_H

#include <intelli/node.h>

#include <gt_stringproperty.h>

namespace intelli
{

class StringBuilderNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE StringBuilderNode();

protected:

    void eval() override;

private:

    PortId m_inA, m_inB, m_out;

    GtStringProperty m_pattern;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGBUILDERNODE_H
