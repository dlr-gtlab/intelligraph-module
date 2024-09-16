/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTSOURCENODE_H
#define GT_INTELLI_OBJECTSOURCENODE_H

#include <intelli/node.h>

#include <intelli/property/objectlink.h>

namespace intelli
{

class ObjectSourceNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ObjectSourceNode();

protected:

    void eval() override;

private:

    /// selected object
    ObjectLinkProperty m_object;

    /// member to keep track of last object pointer to disconnect changed signals
    QPointer<GtObject> m_lastObject;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTSOURCENODE_H
