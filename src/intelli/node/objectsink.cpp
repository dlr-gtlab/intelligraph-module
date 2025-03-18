/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#include "objectsink.h"

#include <intelli/data/object.h>

using namespace intelli;

ObjectSink::ObjectSink() :
    Node("Object sink")
{
    m_in  = addInPort({typeId<ObjectData>(), tr("Object")});
}

void
intelli::ObjectSink::eval()
{
    auto data = nodeData<ObjectData>(m_in);

    if (!data || !data->object())
    {
        return;
    }
}
