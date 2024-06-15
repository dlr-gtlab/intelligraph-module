/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 12.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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

bool
TestNode::handleNodeEvaluation(intelli::NodeDataInterface& model)
{
    return !failEvaluation && intelli::Node::handleNodeEvaluation(model);
}

