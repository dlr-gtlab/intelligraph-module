/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_BOOLINPUTNODE_H
#define GT_INTELLI_BOOLINPUTNODE_H

#include <intelli/node.h>
#include <intelli/property/metaenum.h>

#include <gt_boolproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT BoolInputNode : public Node
{
    Q_OBJECT

    friend class BoolNodeUI;

public:

    Q_INVOKABLE BoolInputNode();

    bool value() const;

    void setValue(bool value);

protected:

    void eval() override;

private:

    GtBoolProperty m_value;

    MetaEnumProperty m_displayMode;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLINPUTNODE_H
