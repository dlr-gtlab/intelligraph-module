/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/objectsource.h"

#include "gt_objectfactory.h"

#include "gt_application.h"
#include "gt_project.h"

#include "intelli/data/object.h"
#include "gt_propertyobjectlinkeditor.h"

using namespace intelli;

// helper method to fetch the correct root object for retrieve object link
auto getObject = [](Node* node) -> GtObject* {
    if (!gtApp) return node;
    auto* project = gtApp->currentProject();
    if (!project) return node;
    return project;
};

ObjectSourceNode::ObjectSourceNode() :
    Node(tr("Object Source")),
    m_object("target", tr("Target"), tr("Target Object"),
             getObject(this), gtObjectFactory->knownClasses())
{
    registerProperty(m_object);

    // to keep compatibility
    m_out = addOutPort(PortInfo::customId(PortId(1), typeId<ObjectData>()));

    registerWidgetFactory([this]() {
        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&m_object);
        w->setScope(gtApp->currentProject());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };
        
        connect(this, &Node::evaluated, w.get(), update);

        update();

        return w;
    });

    connect(&m_object, &GtAbstractProperty::changed,
            this, &ObjectSourceNode::triggerNodeEvaluation);

    // connect changed signals of linked object
    connect(this, &Node::evaluated, this, [this](){

        auto* object = m_object.linkedObject();

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

void
ObjectSourceNode::eval()
{
    auto* linkedObject = m_object.linkedObject();

    m_object.revert();

    if (!linkedObject || !m_object.allowedClasses().contains(linkedObject->metaObject()->className()))
    {
        setNodeData(m_out, nullptr);
        return;
    }

    m_object.setVal(linkedObject->uuid());

    setNodeData(m_out, std::make_shared<ObjectData>(linkedObject));
}
