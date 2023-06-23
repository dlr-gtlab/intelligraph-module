/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraphnodegroup.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupOutputProvider, "Group")

GtIgGroupOutputProvider::GtIgGroupOutputProvider() :
    GtIntelliGraphNode("Output Provider")
{
    setId(NodeId{std::numeric_limits<int>::max() - 1});
    setPos({200, 0});

    addInPort(gt::ig::typeId<GtIgObjectData>());
}

GtIntelliGraphNode::NodeData
GtIgGroupOutputProvider::eval(PortId outId)
{
    auto* group = findParent<GtIntelliGraphNodeGroup*>();
    if (!group)
    {
        gtWarning().medium()
            << tr("Group output evaluation failed! (Cannot access parent group node)");
        return {};
    }

    for (auto const& p : this->ports(PortType::In))
    {
        PortIndex idx = portIndex(PortType::In, p.id());
        if (!group->setOutData(idx, portData(p.id())))
        {
            gtWarning().medium()
                << tr("Failed to forward output data to group node for idx '%1'")
                   .arg(idx);
        }
    }

    return {};
}
