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

#include <gt_stringproperty.h>
#include "abstractinputnode.h"

namespace intelli
{
class GT_INTELLI_EXPORT StringInputNode : public AbstractInputNode
{
    Q_OBJECT
public:
    Q_INVOKABLE StringInputNode();

    QString value() const;

    void setValue(QString const& value);

    void eval() override;
private:
    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGINPUTNODE_H
