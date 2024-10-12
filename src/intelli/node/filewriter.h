/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef FILEWRITERNODE_H
#define FILEWRITERNODE_H

#include <intelli/node.h>

namespace intelli
{

class FileWriterNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FileWriterNode();

protected:

    void eval() override;

private:

    PortId m_inFile, m_inData, m_outSuccess;
};

} // namespace intelli

#endif // FILEWRITERNODE_H
