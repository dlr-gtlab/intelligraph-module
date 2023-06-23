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
        m_outData.push_back(NodeData{});
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

bool
GtIntelliGraphNodeGroup::setOutData(PortIndex idx, NodeData data)
{
    if (idx >= m_outData.size())
    {
        gtError().medium() << tr("Failed to set out data! (Index out of bounds)");
        return false;
    }

    gtDebug().verbose() << "Setting group output data:" << data;

    m_outData.at(idx) = std::move(data);

    updatePort(idx);

    return true;
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNodeGroup::eval(PortId outId)
{
    auto out = outputProvider();
    if (!out)
    {
        gtError().medium() << tr("Failed to evaluate group node! (Invalid output provider)");
        return {};
    };

    auto in = inputProvider();
    if (!in)
    {
        gtError().medium() << tr("Failed to evaluate group node! (Invalid input provider)");
        return {};
    }

    PortIndex idx{0};

    in->updateNode();

//    for (auto const& _ : ports(PortType::In))
//    {
//        if (!in->setOutData(idx, inData(idx)))
//        {
//            gtWarning().medium()
//                << tr("Failed to forward input data idx '%1'")
//                       .arg(idx) << in->ports(PortType::In);
//        }
//        idx++;
//    }

    // idealy now the data should have been set

    if (m_outData.size() != out->ports(PortType::In).size())
    {
        gtWarning().medium()
            << tr("Group out data mismatches output provider! (%1 vs %2)")
               .arg(out->ports(PortType::In).size())
               .arg(m_outData.size());
        return {};
    }

    idx = portIndex(PortType::Out, outId);

    return m_outData.at(idx);
}

