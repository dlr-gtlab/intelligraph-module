/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/general/listgetindex.h"

#include "intelli/data/int.h"
#include "intelli/data/double.h"
#include "intelli/data/list.h"
#include "intelli/nodedatafactory.h"

using namespace intelli;

ListGetIndexNode::ListGetIndexNode() :
    Node(tr("Get Index")),
    m_typeId("typeId",
             tr("Type Id"),
             NodeDataFactory::instance().validTypeIds(),
             typeId<DoubleData>())
{
    registerProperty(m_typeId);

    m_in    = addInPort(makePort(typeId<list<DoubleData>>()).setCaption(tr("list")).setOptional(false));
    m_index = addInPort(makePort(typeId<IntData>()).setCaption(tr("index")).setOptional(false));
    m_out   = addOutPort(makePort(typeId<DoubleData>()).setCaption(tr("entry")));

    auto updatePort = [this](){
        auto& factory = NodeDataFactory::instance();
        if (factory.isListType(m_typeId.get())) return; // invalid

        NodePort* inPort = this->port(m_in);
        NodePort* outPort = this->port(m_out);

        if (inPort && inPort->typeId != factory.listType(m_typeId.get()))
        {
            inPort->typeId = factory.listType(m_typeId.get());
            emit portChanged(inPort->id());
        }

        if (outPort && outPort->typeId != m_typeId.get())
        {
            outPort->typeId = m_typeId.get();
            emit portChanged(outPort->id());
        }
    };

    connect(&m_typeId, &StringSelectionProperty::changed, this, updatePort);
}

void
ListGetIndexNode::eval()
{
    Ptr<IntData> indexData = nodeData<IntData>(m_index);
    if (!indexData) return evalFailed();

    Ptr<ListData> listData = nodeData<ListData>(m_in);
    if (!listData) return evalFailed();

    int index = indexData->value();
    if (static_cast<size_t>(index) >= listData->iterate().size() || index < 0)
    {
        return evalFailed();
    }

    Ptr<NodeData> data = *std::next(listData->iterate().begin(), index);
    setNodeData(m_out, std::move(data));
}

