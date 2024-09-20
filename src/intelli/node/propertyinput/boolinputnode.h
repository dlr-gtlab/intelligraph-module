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

#include <gt_boolproperty.h>

#include "abstractinputnode.h"

namespace intelli
{
class GT_INTELLI_EXPORT BoolInputNode : public AbstractInputNode
{
    Q_OBJECT
public:
    Q_INVOKABLE BoolInputNode();

    bool value() const;

    void setValue(bool value);

    void eval() override;

private:
    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLINPUTNODE_H
