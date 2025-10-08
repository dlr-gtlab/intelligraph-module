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
                QStringList{typeId<InvalidData>()},
                NoUserDynamicInputAndOutput),
    m_object("target", tr("Target"), tr("Target Object"), this, QStringList{})
{
    setFlag(UserRenamable, false);
    setFlag(UserDeletable, false);

    registerProperty(m_object);
    m_object.setReadOnly(true);
    m_object.hide(true);

#ifdef GT_INTELLI_DEBUG_NODE_PROPERTIES
    m_object.hide(false);
#endif

    setNodeEvalMode(NodeEvalMode::Blocking);

    setToolTip(tr("Dummy node: changes cannot be applied!"));
}

bool
DummyNode::setDummyObject(GtObject& object)
{
    if (!m_object.get().isEmpty() || !object.isDummy()) return false;

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
    m_object.setVal(object.uuid());

    return true;
}

QString const&
DummyNode::linkedUuid() const
{
    return m_object.get();
}

GtObject*
DummyNode::linkedObject()
{
    return m_object.linkedObject(parentObject());
}

GtObject const*
DummyNode::linkedObject() const
{
    return m_object.linkedObject(parentObject());
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
