/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 8.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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

