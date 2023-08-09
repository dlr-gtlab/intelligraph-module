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

GT_INTELLI_REGISTER_NODE(GroupOutputProvider, "")

GroupOutputProvider::GroupOutputProvider() :
    AbstractGroupProvider("Output Provider")
{
    setPos({250, 0});
}

Node::NodeDataPtr
GroupOutputProvider::eval(PortId outId)
{
    auto* group = findParent<Graph*>();
    if (!group)
    {
        gtWarning().medium()
            << tr("Group output evaluation failed! (Cannot access parent group node)");
        return {};
    }

    for (auto const& p : this->ports(PortType::In))
    {
        PortIndex idx = portIndex(PortType::In, p.id());
        if (!group->setOutData(idx, nodeData(p.id())))
        {
            gtWarning().medium()
                << tr("Failed to forward output data to group node for idx '%1'")
                   .arg(idx);
        }
    }

    return {};
}
