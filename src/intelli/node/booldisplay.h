/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLDISPLAYNODE_H
#define GT_INTELLI_BOOLDISPLAYNODE_H

#include <intelli/node.h>

#include <intelli/property/metaenum.h>

namespace intelli
{

class BoolDisplayNode : public Node
{
    Q_OBJECT

    friend class BoolNodeUI;

public:

    Q_INVOKABLE BoolDisplayNode();

private:

    MetaEnumProperty m_displayMode;

    PortId m_in;
};

} // namespace intelli

#endif // BOOLDISPLAYNODE_H
