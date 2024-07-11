/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
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
