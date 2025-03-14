/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/dummy.h>

#include <gt_objectmemento.h>

using namespace intelli;

DummyNode::DummyNode() :
    Node("Dummy Node")
{
    setNodeEvalMode(NodeEvalMode::Blocking);
}

bool
DummyNode::setDummyObject(GtObject& object)
{
    if (!object.isDummy()) return false;

    GtObjectMemento memento = object.toMemento();

    for (auto& prop : memento.properties)
    {
        // apply node id
        if (prop.name == "id") setId(NodeId::fromValue(prop.data().toUInt()));
        // apply position
        if (prop.name == "posX") setPos(Position{prop.data().toDouble(), pos().y()});
        if (prop.name == "posY") setPos(Position{pos().x(), prop.data().toDouble()});
    }

    setCaption(memento.ident());

    return true;
}

void
DummyNode::eval()
{
    return evalFailed();
}

DummyData::DummyData() :
    NodeData("n/a")
{ }
