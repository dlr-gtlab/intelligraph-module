/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/groupoutputprovider.h>

using namespace intelli;

GroupOutputProvider::GroupOutputProvider() :
    AbstractGroupProvider("Output Provider")
{
    setPos({250, 0});
}

void
GroupOutputProvider::eval()
{
    auto const& inPorts  = ports(PortType::In);
    auto const& outPorts = ports(PortType::Out);

    assert(inPorts.size() == outPorts.size());

    for (auto& port : inPorts)
    {
        setNodeData(virtualPortId(port.id()), nodeData(port.id()));
    }
}
