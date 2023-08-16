/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/groupoutputprovider.h"
#include "intelli/nodefactory.h"

using namespace intelli;

static auto init_once = [](){
    return GT_INTELLI_REGISTER_NODE(GroupOutputProvider, "")
}();

GroupOutputProvider::GroupOutputProvider() :
    AbstractGroupProvider("Output Provider")
{
    setPos({250, 0});
}

Node::NodeDataPtr
GroupOutputProvider::eval(PortId outId)
{
    return {};
}
