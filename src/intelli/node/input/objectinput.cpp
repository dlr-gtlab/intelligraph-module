/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/input/objectinput.h>
#include <intelli/data/object.h>
#include <intelli/nodedatainterface.h>

using namespace intelli;

constexpr bool s_useSuperClass = true;

ObjectInputNode::ObjectInputNode() :
    Node("Object Input"),
    m_object("target", tr("Target"), tr("Target Object"),
             this, QStringList{GT_CLASSNAME(GtObject)}, s_useSuperClass)
{
    registerProperty(m_object);

    setNodeFlag(ResizableHOnly);

    m_out = addOutPort(makePort(typeId<ObjectData>())
                           .setCaptionVisible(false));

    connect(&m_object, &GtAbstractProperty::changed,
            this, &ObjectInputNode::triggerNodeEvaluation);

    // connect changed signals of linked object
    connect(this, &Node::evaluated, this, [this](){

        auto* object = linkedObject();

        if (m_lastObject && m_lastObject != object)
        {
            this->disconnect(m_lastObject);
        }

        if (object)
        {
            connect(object, qOverload<GtObject*>(&GtObject::dataChanged),
                    this, &Node::triggerNodeEvaluation, Qt::UniqueConnection);
            connect(object, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                    this, &Node::triggerNodeEvaluation, Qt::UniqueConnection);
        }
    });
}

GtObjectLinkProperty&
ObjectInputNode::objectProperty()
{
    return m_object;
}

GtObjectLinkProperty const&
ObjectInputNode::objectProperty() const
{
    return m_object;
}

GtObject*
ObjectInputNode::linkedObject(GtObject* root)
{
    if (!root)
    {
        auto* model = exec::nodeDataInterface(*this);
        if (model) root = model->scope();
    }
    return m_object.linkedObject(root);
}

GtObject const*
ObjectInputNode::linkedObject(GtObject const* root) const
{
    return const_cast<ObjectInputNode*>(this)
        ->linkedObject(const_cast<GtObject*>(root));
}

void
ObjectInputNode::setValue(QString const& uuid)
{
    if (m_object.getVal() == uuid) return;
    m_object.setVal(uuid);
}

void
ObjectInputNode::eval()
{
    auto* linkedObj = linkedObject();

    m_object.revert();

    if (!linkedObj)
    {
        setNodeData(m_out, nullptr);
        return;
    }

    setValue(linkedObj->uuid());

    setNodeData(m_out, std::make_shared<intelli::ObjectData>(linkedObj));
}
