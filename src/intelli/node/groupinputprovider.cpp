/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/groupinputprovider.h"
#include "intelli/nodefactory.h"

using namespace intelli;

GTIG_REGISTER_NODE(GroupInputProvider, "")

GroupInputProvider::GroupInputProvider() :
    AbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});
}

Node::NodeDataPtr
GroupInputProvider::eval(PortId outId)
{
    PortIndex idx = portIndex(PortType::Out, outId);

    if (idx == invalid<PortIndex>()) return {};
    
    auto* group = findParent<Graph*>();
    if (!group)
    {
        gtWarning().medium()
            << tr("Group input evaluation failed! (Cannot access parent group node)");
        return {};
    }

    return group->nodeData(group->portId(PortType::In, idx));
}
