/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/general/listgetsize.h"

#include "intelli/data/int.h"
#include "intelli/data/double.h"
#include "intelli/data/list.h"
#include "intelli/nodedatafactory.h"

using namespace intelli;

ListGetSizeNode::ListGetSizeNode() :
    Node(tr("Get Size")),
    m_typeId("typeId",
             tr("Type Id"),
             NodeDataFactory::instance().validTypeIds(),
             typeId<DoubleData>())
{
    registerProperty(m_typeId);

    m_in  = addInPort(makePort(typeId<list<DoubleData>>()).setCaption(tr("list")));
    m_out = addOutPort(makePort(typeId<IntData>()).setCaption(tr("size")));

    auto updatePort = [this](){
        auto& factory = NodeDataFactory::instance();
        if (factory.isListType(m_typeId.get())) return; // invalid

        NodePort* inPort = this->port(m_in);

        if (inPort && inPort->typeId != factory.listType(m_typeId.get()))
        {
            inPort->typeId = factory.listType(m_typeId.get());
            emit portChanged(inPort->id());
        }
    };

    connect(&m_typeId, &StringSelectionProperty::changed, this, updatePort);
}

void
ListGetSizeNode::eval()
{
    int size = 0;

    Ptr<BaseListData> listData = nodeData<BaseListData>(m_in);
    if (listData)
    {
        size = listData->iterate().size();
    }

    setNodeData(m_out, makeNodeData<IntData>(size));
}
