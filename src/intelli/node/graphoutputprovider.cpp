/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/graphoutputprovider.h>

#include <intelli/nodeevent.h>

using namespace intelli;

void
GraphOutputProvider::nodeEvent(const NodeEvent* e)
{
    if (e->cast<NodeInvalidatedEvent>())
    {
        emit nodeInvalidated();
    }
    return Node::nodeEvent(e);
}
