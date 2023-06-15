/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnodegroup.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraph.h"

#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"

GTIG_REGISTER_NODE(GtIntelliGraphNodeGroup, "Group")

GtIntelliGraphNodeGroup::GtIntelliGraphNodeGroup() :
    GtIntelliGraphNode("Group Node")
{
    auto* graph = new GtIntelliGraph();
    graph->setParent(this);
    graph->setDefault(true);

    auto input = std::make_unique<GtIgGroupInputProvider>();
    input->setDefault(true);

    for (auto const& data : input->ports(PortType::Out))
    {
        addInPort(data);
    }

    input.release()->setParent(graph);

    auto output = std::make_unique<GtIgGroupOutputProvider>();
    output->setDefault(true);
    output->setParent(graph);

    for (auto const& data : output->ports(PortType::In))
    {
        addOutPort(data);
    }
    
    connect(output.get(), &GtIntelliGraphNode::outDataUpdated,
            this, &GtIntelliGraphNode::outDataUpdated);
    connect(output.get(), &GtIntelliGraphNode::outDataInvalidated,
            this, &GtIntelliGraphNode::outDataInvalidated);

    output.release();
}

GtIntelliGraph*
GtIntelliGraphNodeGroup::graph()
{
    return findDirectChild<GtIntelliGraph*>();
}

GtIntelliGraph const*
GtIntelliGraphNodeGroup::graph() const
{
    return const_cast<GtIntelliGraphNodeGroup*>(this)->graph();
}

GtIgGroupInputProvider*
GtIntelliGraphNodeGroup::inputProvider()
{
    auto graph = this->graph();
    return graph ? graph->findDirectChild<GtIgGroupInputProvider*>() : nullptr;
}

GtIgGroupInputProvider const*
GtIntelliGraphNodeGroup::inputProvider() const
{
    return const_cast<GtIntelliGraphNodeGroup*>(this)->inputProvider();
}

GtIgGroupOutputProvider*
GtIntelliGraphNodeGroup::outputProvider()
{
    auto graph = this->graph();
    return graph ? graph->findDirectChild<GtIgGroupOutputProvider*>() : nullptr;
}

GtIgGroupOutputProvider const*
GtIntelliGraphNodeGroup::outputProvider() const
{
    return const_cast<GtIntelliGraphNodeGroup*>(this)->outputProvider();
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNodeGroup::eval(PortId id)
{
//    auto out = outputProvider();
//    if (!out)
//    {
//        gtError().medium() << tr("Invalid output provider!");
//        return {};
//    };

//    auto in = inputProvider();
//    if (!in)
//    {
//        gtError().medium() << tr("Invalid input provider!");
//        return {};
//    }

//    auto* inPort = in->port(portIndex(PortType::In, id));
//    if (!inPort)
//    {
//        gtError().medium() << tr("Invalid input port!") << id;
//        return {};
//    }

//    in->eval(inPort->id(), data);
    return {};
}

