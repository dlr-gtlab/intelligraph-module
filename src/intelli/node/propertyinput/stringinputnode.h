/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef STRINGINPUTNODE_H
#define STRINGINPUTNODE_H

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
#endif // STRINGINPUTNODE_H
