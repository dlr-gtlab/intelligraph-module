/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
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
