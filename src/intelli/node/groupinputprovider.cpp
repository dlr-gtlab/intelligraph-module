/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/node/groupinputprovider.h>

using namespace intelli;

GroupInputProvider::GroupInputProvider() :
    AbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});
}

void
GroupInputProvider::eval()
{
    auto const& inPorts  = ports(PortType::In);
    auto const& outPorts = ports(PortType::Out);

    assert(inPorts.size() == outPorts.size());

    for (auto& port : inPorts)
    {
        setNodeData(mainPortId(port.id()), nodeData(port.id()));
    }
}
