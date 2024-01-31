/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 22.01.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef GT_INTELLI_EXISTINGDIRECTORYSOURCE_H
#define GT_INTELLI_EXISTINGDIRECTORYSOURCE_H

#include <intelli/node.h>

#include <gt_existingdirectoryproperty.h>

namespace intelli
{
class ExistingDirectorySourceNode : public Node
{
    Q_OBJECT
public:
    Q_INVOKABLE ExistingDirectorySourceNode();

    void eval() override;

private:
    GtExistingDirectoryProperty m_value;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_EXISTINGDIRECTORYSOURCE_H
