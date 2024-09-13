/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef TESTNODE_H
#define TESTNODE_H

#include <intelli/node.h>
#include <intelli/node/sleepy.h>

class TestNode : public intelli::Node
{
    Q_OBJECT

public:

    bool failEvaluation = false;

    static void registerOnce();

    Q_INVOKABLE TestNode();

    using Node::setFlag;
    using Node::setNodeEvalMode;
    using Node::insertInPort;
    using Node::insertOutPort;
    using Node::addInPort;
    using Node::addOutPort;
    using Node::removePort;

protected:

    bool handleNodeEvaluation(intelli::NodeDataInterface& model) override;
};

class TestSleepyNode : public intelli::SleepyNode
{
public:

    static void registerOnce();

    Q_INVOKABLE TestSleepyNode();

    using Node::setFlag;
    using Node::setNodeEvalMode;
};

#endif // TESTNODE_H
