/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/objectmemento.h>

#include <gt_objectmemento.h>

#include <intelli/data/object.h>
#include <intelli/data/bytearray.h>

#include <QLayout>

using namespace intelli;

ObjectMementoNode::ObjectMementoNode() :
    Node("To Memento")
{
    m_in  = addInPort(makePort(typeId<ObjectData>()).setOptional(false));
    m_out = addOutPort(makePort(typeId<ByteArrayData>()).setCaption("memento"));
}

void
ObjectMementoNode::eval()
{
    auto data = nodeData<ObjectData>(m_in);

    if (!data || !data->object())
    {
        setNodeData(m_out, nullptr);
        return;
    }

    setNodeData(m_out, std::make_shared<ByteArrayData>(data->object()->toMemento().toByteArray()));
}
