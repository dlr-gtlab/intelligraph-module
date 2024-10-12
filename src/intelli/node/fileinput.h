/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
