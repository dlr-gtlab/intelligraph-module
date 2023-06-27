/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupinputprovider.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraphnodegroup.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupInputProvider, "")

GtIgGroupInputProvider::GtIgGroupInputProvider() :
    GtIgAbstractGroupProvider("Input Provider")
{
    setId(NodeId{0});
    setPos({-250, 0});
}

GtIntelliGraphNode::NodeData
GtIgGroupInputProvider::eval(PortId outId)
{
    PortIndex idx = portIndex(PortType::Out, outId);

    if (idx == gt::ig::invalid<PortIndex>()) return {};

    auto* group = findParent<GtIntelliGraphNodeGroup*>();
    if (!group)
    {
        gtWarning().medium()
            << tr("Group input evaluation failed! (Cannot access parent group node)");
        return {};
    }

    return group->portData(group->portId(PortType::In, idx));
}
