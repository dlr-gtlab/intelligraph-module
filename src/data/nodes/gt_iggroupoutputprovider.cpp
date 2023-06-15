/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupOutputProvider, "Group")

GtIgGroupOutputProvider::GtIgGroupOutputProvider() :
    GtIntelliGraphNode("Output Provider")
{
    setId(std::numeric_limits<int>::max() - 1);
    setPos({200, 0});

    addInPort(gt::ig::typeId<GtIgObjectData>());
}

GtIntelliGraphNode::NodeData
GtIgGroupOutputProvider::eval(PortId outId)
{
    return {};
//    return portData(portId(PortType::In, portIndex(PortType::Out, outId)));
}
