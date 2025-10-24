/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/binarydisplay.h"
#include "intelli/data/bool.h"

using namespace intelli;

BinaryDisplayNode::BinaryDisplayNode() :
    DynamicNode(tr("Binary Display"),
                QStringList{typeId<BoolData>()},
                QStringList{},
                DynamicNode::DynamicInput)
{
    setNodeEvalMode(NodeEvalMode::Blocking);
    setNodeFlag(Resizable);

    addStaticInPort(makePort(typeId<BoolData>()).setCaption("in_0"));
}

unsigned int
BinaryDisplayNode::inputValue()
{
    unsigned value = 0u;
    size_t idx  = 0;
    for (PortInfo const& info : ports(PortType::In))
    {
        if (auto data = nodeData<BoolData>(info.id()))
        {
            value |= (unsigned)data->value() << idx;
        }
        idx++;
    }
    return value;
}
