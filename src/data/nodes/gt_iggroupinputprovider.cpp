/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupinputprovider.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupInputProvider, "Group")

GtIgGroupInputProvider::GtIgGroupInputProvider() :
    GtIntelliGraphNode("Input Provider")
{
    setId(NodeId{std::numeric_limits<int>::max() - 2});
    setPos({-200, 0});

    addOutPort(gt::ig::typeId<GtIgObjectData>());
}

GtIntelliGraphNode::NodeData
GtIgGroupInputProvider::eval(PortId outId)
{
    return {};
//    return portData(portId(PortType::In, portIndex(PortType::Out, outId)));
}
