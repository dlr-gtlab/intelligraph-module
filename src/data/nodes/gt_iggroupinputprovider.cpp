/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_iggroupinputprovider.h"
#include "gt_intelligraphnodefactory.h"

#include "models/data/gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgGroupInputProvider, "Group")

GtIgGroupInputProvider::GtIgGroupInputProvider() :
    GtIntelliGraphNode("Input Provider")
{
    setId(std::numeric_limits<int>::max() - 2);
    setPos({-200, 0});
}

unsigned
GtIgGroupInputProvider::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::Out:
        return 1;
    case PortType::In:
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeDataType
GtIgGroupInputProvider::dataType(PortType type, PortIndex idx) const
{
    switch (type)
    {
    case PortType::Out:
        return GtIgObjectData::staticType();
    case PortType::In:
    case PortType::None:
        return {};
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeData
GtIgGroupInputProvider::outData(const PortIndex port)
{
    if (port >= m_data.size())
    {
        gtWarningId("GtIgGroupInputProvider") << tr("Output port out of bounds!") << port << m_data.size();
        return {};
    }

    return m_data[port];
}

void
GtIgGroupInputProvider::setInData(NodeData data, const PortIndex port)
{
    auto ports = nPorts(PortType::Out);

    if (port >= ports)
    {
        gtWarningId("GtIgGroupInputProvider") << tr("Input port out of bounds!") << port << ports;
        return;
    }
    m_data.resize(ports);

    m_data[port] = std::move(data);

    emit dataUpdated(port);
}

