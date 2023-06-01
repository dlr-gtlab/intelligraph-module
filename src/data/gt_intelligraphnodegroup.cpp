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
#include "nodes/gt_iggroupinputprovider.h"
#include "nodes/gt_iggroupoutputprovider.h"

GTIG_REGISTER_NODE(GtIntelliGraphNodeGroup, "Group")

GtIntelliGraphNodeGroup::GtIntelliGraphNodeGroup() :
    GtIntelliGraphNode("Group Node")
{
    auto* graph = new GtIntelliGraph();
    graph->setParent(this);
    graph->setDefault(true);

    auto input = std::make_unique<GtIgGroupInputProvider>();
    input->setDefault(true);
    input.release()->setParent(graph);

    auto output = std::make_unique<GtIgGroupOutputProvider>();
    output->setDefault(true);
    output->setParent(graph);

    connect(output.get(), &GtIntelliGraphNode::dataUpdated,
            this, &GtIntelliGraphNode::dataUpdated);
    connect(output.get(), &GtIntelliGraphNode::dataInvalidated,
            this, &GtIntelliGraphNode::dataInvalidated);

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

unsigned
GtIntelliGraphNodeGroup::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        if (auto* provider = this->inputProvider())
        {
            return provider->nPorts(PortType::Out);
        }
        return 0;
    case PortType::Out:
        if (auto* provider  = this->outputProvider())
        {
            return provider->nPorts(PortType::In);
        }
        return 0;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeDataType
GtIntelliGraphNodeGroup::dataType(PortType type, PortIndex port) const
{
    switch (type)
    {
    case PortType::In:
        if (auto* inputProvider = this->inputProvider())
        {
            return inputProvider->dataType(PortType::Out, port);
        }
        return {};
    case PortType::Out:
        if (auto* outputProvider = this->outputProvider())
        {
            return outputProvider->dataType(PortType::In, port);
        }
        return {};
    case PortType::None:
        return {};
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNodeGroup::outData(const PortIndex port)
{
    auto provider = outputProvider();
    if (!provider) return {};

    return provider->outData(port);
}

void
GtIntelliGraphNodeGroup::setInData(NodeData data, const PortIndex port)
{
    auto provider = inputProvider();
    if (!provider) return;

    provider->setInData(data, port);
}

