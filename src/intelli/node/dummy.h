/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DUMMYNODE_H
#define GT_INTELLI_DUMMYNODE_H

#include <intelli/node.h>
#include <intelli/nodedata.h>

namespace intelli
{

class DummyNode : public intelli::Node
{
    Q_OBJECT

public:

    Q_INVOKABLE DummyNode();

    using Node::addInPort;
    using Node::addOutPort;

    bool setDummyObject(GtObject& object);

protected:

    void eval() override;
};

class DummyData : public intelli::NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE DummyData();
};

} // namespace intelli

#endif // GT_INTELLI_DUMMYNODE_H
