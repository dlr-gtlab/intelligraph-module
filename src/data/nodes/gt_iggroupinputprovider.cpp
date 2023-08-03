/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupinputprovider.h"
#include "gt_intelligraphnodefactory.h"

GTIG_REGISTER_NODE(GtIgGroupInputProvider, "")

GtIgGroupInputProvider::GtIgGroupInputProvider() :
    GtIgAbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});
}

GtIntelliGraphNode::NodeData
GtIgGroupInputProvider::eval(PortId outId)
{
    PortIndex idx = portIndex(PortType::Out, outId);

    if (idx == gt::ig::invalid<PortIndex>()) return {};

    auto* group = findParent<GtIntelliGraph*>();
    if (!group)
    {
        gtWarning().medium()
            << tr("Group input evaluation failed! (Cannot access parent group node)");
        return {};
    }

    return group->nodeData(group->portId(PortType::In, idx));
}
