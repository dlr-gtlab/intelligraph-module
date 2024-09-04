/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 12.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
