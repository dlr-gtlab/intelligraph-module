/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
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
