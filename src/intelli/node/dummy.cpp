/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/dummy.h>
#include <intelli/data/invalid.h>

#include <gt_objectmemento.h>

using namespace intelli;

DummyNode::DummyNode() :
    DynamicNode("Dummy Node",
                QStringList{typeId<InvalidData>()},
                QStringList{typeId<InvalidData>()}),
    m_object("target", tr("Target"), tr("Target Object"), this, QStringList{})
{
    registerProperty(m_object);
    m_object.setReadOnly(true);
    m_object.hide(true);

#ifdef INTELLIGRAPH_DEBUG_NODE_PROPERTIES
    m_object.setReadOnly(false);
    m_object.setHidden(false);
#endif

    setNodeEvalMode(NodeEvalMode::Blocking);

    setToolTip(tr("Unkown node, changes cannot be applied to linked node!"));
}

bool
DummyNode::setDummyObject(GtObject& object)
{
    if (!object.isDummy()) return false;

    GtObjectMemento memento = object.toMemento();

    for (auto& prop : memento.properties)
    {
        // apply node id
        if (prop.name == QStringLiteral("id")) setId(NodeId::fromValue(prop.data().toUInt()));
        // apply position
        if (prop.name == QStringLiteral("posX")) setPos(Position{prop.data().toDouble(), pos().y()});
        if (prop.name == QStringLiteral("posY")) setPos(Position{pos().x(), prop.data().toDouble()});
    }

    setCaption(memento.ident() + QStringLiteral("[?]"));

    return true;
}

QString const&
DummyNode::linkedUuid() const
{
    return m_object.get();
}

void
DummyNode::eval()
{
    return evalFailed();
}

void
DummyNode::onObjectDataMerged()
{
    DynamicNode::onObjectDataMerged();
}
