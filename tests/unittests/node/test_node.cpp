/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_node.h"

#include <intelli/nodefactory.h>

void
TestNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestNode, "Test");
    }();
    Q_UNUSED(_);
}

TestNode::TestNode() :
    TestNode("MyNode")
{

}

TestNode::TestNode(QString modelName) :
    intelli::Node(std::move(modelName))
{

}

void
TestNode::eval()
{
    if (failEvaluation)
    {
        gtWarning().nospace() << caption() << ": "
                              << tr("Triggering node failure!");
        evalFailed();
    }
}

void
TestSleepyNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestNode, "Test");
    }();
    Q_UNUSED(_);
}

void
TestNumberInputNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestNumberInputNode, "Test");
    }();
    Q_UNUSED(_);
}

TestNumberInputNode::TestNumberInputNode() :
    intelli::DoubleInputNode()
{
    setNodeEvalMode(NodeEvalMode::Detached); // for unittesting
}

TestUnregisteredNode::TestUnregisteredNode() :
    TestNode("Unregistered Node")
{

}

TestDeprecatedNode::TestDeprecatedNode() :
    TestNode("Deprecated Node")
{
    setNodeFlag(intelli::Deprecated);
}
