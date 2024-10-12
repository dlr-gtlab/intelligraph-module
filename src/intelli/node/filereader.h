/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FILEREADERNODE_H
#define GT_INTELLI_FILEREADERNODE_H

#include <intelli/node.h>

namespace intelli
{

class FileReaderNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FileReaderNode();

protected:

    void eval() override;

private:

    PortId m_inFile, m_outData;
};

} // namespace intelli

#endif // GT_INTELLI_FILEREADERNODE_H
