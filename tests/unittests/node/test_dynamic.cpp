/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_dynamic.h"

#include "intelli/nodefactory.h"

void
TestDynamicNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestDynamicNode, "Test");
    }();
    Q_UNUSED(_);
}

TestDynamicNode::TestDynamicNode() :
    intelli::DynamicNode("MyDynamicNode")
{

}

void
TestDynamicWhiteListNode::registerOnce()
{
    static auto _ = []{
        return GT_INTELLI_REGISTER_NODE(TestDynamicWhiteListNode, "Test");
    }();
    Q_UNUSED(_);
}

TestDynamicWhiteListNode::TestDynamicWhiteListNode(QStringList inputWhiteList,
                                                   QStringList outputWhiteList) :
    intelli::DynamicNode("MyDynamicWhiteListNode", inputWhiteList, outputWhiteList)
{

}

