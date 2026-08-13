/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/groupoutputprovider.h>
#include "intelli/graphexecmodel.h"

using namespace intelli;

GroupOutputProvider::GroupOutputProvider() :
    GroupOutputProvider("Output Provider")
{ }


GroupOutputProvider::GroupOutputProvider(QString const& modelName) :
    AbstractGroupProvider(modelName)
{
    setPos({250, 0});
}

void
GroupOutputProvider::eval()
{
    auto* interface = exec::nodeDataInterface(*this);
    if (qobject_cast<GraphExecutionModel*>(interface))
    {
        auto const& inPorts  = ports(PortType::In);
        auto const& outPorts = ports(PortType::Out);

        assert(inPorts.size() == outPorts.size());

        for (auto& port : inPorts)
        {
            setNodeData(virtualPortId(port.id()), nodeData(port.id()));
        }
        return;
    }
}
