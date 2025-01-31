/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_STRINGINPUTNODE_H
#define GT_INTELLI_STRINGINPUTNODE_H

#include <intelli/node.h>

#include <gt_stringproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT StringInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE StringInputNode();

    QString const& value() const;

    void setValue(QString value);

protected:

    void eval() override;

private:

    GtStringProperty m_value;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGINPUTNODE_H
