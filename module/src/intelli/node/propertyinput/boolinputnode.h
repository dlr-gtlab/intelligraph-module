/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef BOOLINPUTNODE_H
#define BOOLINPUTNODE_H

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
#endif // NPBOOLINPUTNODE_H
