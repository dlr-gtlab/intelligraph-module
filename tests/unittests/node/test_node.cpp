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
    intelli::Node("MyNode")
{

}

void
TestSleepyNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestNode, "Test");
    }();
    Q_UNUSED(_);
}
