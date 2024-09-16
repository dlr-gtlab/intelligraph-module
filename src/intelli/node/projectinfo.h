/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_PROJECTINFONODE_H
#define GT_INTELLI_PROJECTINFONODE_H

#include <intelli/node.h>

namespace intelli
{


class ProjectInfoNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ProjectInfoNode();

protected:

    void eval() override;

private:

    PortId m_outPath, m_outName;
};

} // namespace intelli

#endif // GT_INTELLI_PROJECTINFONODE_H
