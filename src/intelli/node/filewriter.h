/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 11.7.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
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
