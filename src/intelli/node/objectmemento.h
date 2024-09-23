/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTMEMENTONODE_H
#define GT_INTELLI_OBJECTMEMENTONODE_H

#include <intelli/node.h>

namespace intelli
{

/**
 * @brief Extracts the memento of an object and outputs it as a string
 */
class ObjectMementoNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ObjectMementoNode();

protected:

    void eval() override;

private:

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTMEMENTONODE_H
