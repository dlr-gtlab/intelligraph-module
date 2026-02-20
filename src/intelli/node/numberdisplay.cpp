/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/numberdisplay.h"

#include "intelli/data/double.h"

using namespace intelli;


NumberDisplayNode::NumberDisplayNode() :
    Node("Number Display")
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    addInPort(makePort(typeId<DoubleData>()).setCaptionVisible(false));

    setNodeFlag(ResizableHOnly);
}

double
NumberDisplayNode::displayValue() const
{
    auto const& inPorts = ports(PortType::In);
    if (inPorts.empty()) return 0;

    auto const& data = nodeData<DoubleData>(inPorts.front().id());
    return data ? data->value() : 0;
}
