/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/groupinputprovider.h>

using namespace intelli;

GroupInputProvider::GroupInputProvider() :
    GroupInputProvider("Input Provider")
{ }


GroupInputProvider::GroupInputProvider(QString const& modelName) :
    AbstractGroupProvider(modelName)
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
