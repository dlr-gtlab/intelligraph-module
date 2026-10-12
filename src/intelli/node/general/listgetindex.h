/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LISTGETINDEXNODE_H
#define GT_INTELLI_LISTGETINDEXNODE_H

#include <intelli/node.h>
#include <intelli/property/stringselection.h>

namespace intelli
{

class ListGetIndexNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ListGetIndexNode();

protected:

    void eval() override;

private:

    PortId m_in, m_index, m_out;
    StringSelectionProperty m_typeId;
};

} // namespace intelli

#endif // GT_INTELLI_LISTGETINDEXNODE_H
