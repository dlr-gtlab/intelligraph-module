/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraph.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupOutputProvider, "")

GtIgGroupOutputProvider::GtIgGroupOutputProvider() :
    GtIgAbstractGroupProvider("Output Provider")
{
    setPos({250, 0});

//    connect(this, &GtIntelliGraphNode::outDataUpdated,
//            this, [t = this](PortIndex idx){
//        if (auto* graph = t->findParent<GtIntelliGraph*>())
//        {
//            graph->outDataUpdated(idx);
//        }
//    });
//    connect(this, &GtIntelliGraphNode::outDataInvalidated,
//            this, [t = this](PortIndex idx){
//        if (auto* graph = t->findParent<GtIntelliGraph*>())
//        {
//            graph->outDataInvalidated(idx);
//        }
//    });
}

GtIntelliGraphNode::NodeData
GtIgGroupOutputProvider::eval(PortId outId)
{
    auto* group = findParent<GtIntelliGraph*>();
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
