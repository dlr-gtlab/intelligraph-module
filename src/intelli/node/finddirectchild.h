/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FINDDIRECTCHILDNODE_H
#define GT_INTELLI_FINDDIRECTCHILDNODE_H

#include <intelli/node.h>

#include <gt_stringproperty.h>

namespace intelli
{

class FindDirectChildNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FindDirectChildNode();

protected:

    void eval() override;

private:

    /// target class name
    GtStringProperty m_childClassName;

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_FINDDIRECTCHILDNODE_H
