/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/general/tolist.h"

#include "intelli/data/double.h"
#include "intelli/data/list.h"
#include "intelli/nodedatafactory.h"
#include "intelli/private/utils.h"

using namespace intelli;

ToListNode::ToListNode() :
    DynamicNode("To List", DynamicInput | NoDefaultListTypes),
    m_typeId("typeId",
             tr("Type Id"),
             NodeDataFactory::instance().validTypeIds(),
             typeId<DoubleData>())
{
    auto updatePort = [this](PortType type, PortIndex idx){
        TypeId const typeId = (type != PortType::Out) ?
                                  m_typeId.get() :
                                  NodeDataFactory::instance().listType(m_typeId.get());

        if (typeId.isEmpty()) return;

        PortId const portId = this->portId(type, idx);
        NodePort* port = this->port(portId);
        assert(port);
        port->optional = false;

        bool update = false;
        if (port->typeId != typeId)
        {
            port->typeId = typeId;
            update = true;
        }

        if (type == PortType::In && port->caption != QString::number(idx + 1))
        {
            port->caption = QString::number(idx + 1);
            update = true;
        }

        if (update) emit portChanged(portId);
    };

    auto updatePorts = [this, updatePort](){
        if (NodeDataFactory::instance().isListType(m_typeId.get())) return;

        PortIndex idx{0};
        for (NodePort const& _ : ports(PortType::In))
        {
            Q_UNUSED(_);
            updatePort(PortType::In, idx++);
        }

        idx = PortIndex{0};
        for (NodePort const& _ : ports(PortType::Out))
        {
            Q_UNUSED(_);
            updatePort(PortType::Out, idx++);
        }
    };

    connect(&m_typeId, &StringSelectionProperty::changed, this, updatePorts);
    connect(this, &DynamicNode::portInserted, this, updatePort);

    m_out = addStaticOutPort(makePort(listTypeId<DoubleData>()).setCaption("out"));
    addStaticInPort(makePort(typeId<DoubleData>()));

    registerProperty(m_typeId);
}

void
ToListNode::eval()
{
    NonConstPtr<ListData> outputData =
        NodeDataFactory::instance().makeListData(m_typeId);
    if (!outputData)
    {
        gtError() << utils::logId(*this)
                  << tr("Failed to generate list type for '%1'!").arg(m_typeId);
        return evalFailed();
    }

    for (NodePort const& port : ports(PortType::In))
    {
        auto data = nodeData(port.id());
        if (!data)
        {
            gtError() << utils::logId(*this)
                      << tr("Data at input port %1 is null!").arg(port.id());
            return evalFailed();
        }
        outputData->append(std::move(data));
    }

    if (outputData->iterate().size() != ports(PortType::In).size())
    {
        gtError() << utils::logId(*this)
                  << tr("Failed to append data!");
        return evalFailed();
    }

    setNodeData(m_out, outputData);
}

Node::PortId
ToListNode::insertPort(PortOption option, PortType type, PortInfo port, int idx)
{
    port.typeId = m_typeId;
    return DynamicNode::insertPort(option, type, std::move(port), idx);
}

