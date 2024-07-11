/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_FILEINPUTNODE_H
#define GT_INTELLI_FILEINPUTNODE_H

#include <intelli/node.h>

#include <gt_openfilenameproperty.h>

namespace intelli
{

class FileInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FileInputNode();

protected:

    void eval() override;

private:

    PortId m_inDir, m_inName, m_outFile;
    GtOpenFileNameProperty m_fileChooser;
};

} // namespace intelli

#endif // GT_INTELLI_FILEINPUTNODE_H
