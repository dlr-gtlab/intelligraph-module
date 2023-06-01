/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupoutputprovider.h"
#include "gt_intelligraphnodefactory.h"

#include "models/data/gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupOutputProvider, "Group")

GtIgGroupOutputProvider::GtIgGroupOutputProvider() :
    GtIntelliGraphNode("Output Provider")
{
    setId(std::numeric_limits<int>::max() - 1);
    setPos({200, 0});
}

unsigned
GtIgGroupOutputProvider::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        return 1;
    case PortType::Out:
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}


GtIntelliGraphNode::NodeDataType
GtIgGroupOutputProvider::dataType(PortType type, PortIndex idx) const
{
    switch (type)
    {
    case PortType::In:
        return GtIgObjectData::staticType();
    case PortType::Out:
    case PortType::None:
        return {};
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeData
GtIgGroupOutputProvider::outData(const PortIndex port)
{
    if (port >= m_data.size())
    {
        gtWarningId("GtIgGroupOutputProvider") << tr("Output port out of bounds!") << port << m_data.size();
        return {};
    }

    return m_data[port];
}

void
GtIgGroupOutputProvider::setInData(NodeData data, const PortIndex port)
{
    auto ports = nPorts(PortType::In);

    if (port >= ports)
    {
        gtWarningId("GtIgGroupOutputProvider") << tr("Input port out of bounds!") << port << ports;
        return;
    }

    m_data.resize(ports);

    m_data[port] = std::move(data);

    emit dataUpdated(port);
}
