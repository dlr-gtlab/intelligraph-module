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

#include <gt_application.h>
#include <gt_project.h>

#include <gt_propertyobjectlinkeditor.h>

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

    registerWidgetFactory([this]() {
        auto* model = exec::nodeDataInterface(*this);

        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&m_object);
        w->setScope(model ? model->scope() : m_object.object());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };

        connect(this, &Node::evaluated, w.get(), update);

        update();

        return w;
    });

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
