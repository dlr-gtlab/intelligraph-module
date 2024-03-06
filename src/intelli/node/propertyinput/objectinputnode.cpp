/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "objectinputnode.h"
#include <intelli/data/object.h>

#include "gt_application.h"
#include "gt_project.h"

#include "gt_propertyobjectlinkeditor.h"

namespace intelli
{
// helper method to fetch the correct root object for retrieve object link
auto getObject = []() -> GtObject* {
    if (!gtApp) return nullptr;
    auto* project = gtApp->currentProject();
    if (!project) return nullptr;
    return project;
};

ObjectInputNode::ObjectInputNode() :
    AbstractInputNode("Object Input",
                      std::make_unique<GtObjectLinkProperty>(
                          QString("value"),
                          tr("Value"),
                          tr("Current Value"),
                          QString(""),
                          getObject(),
                          QStringList{GT_CLASSNAME(GtObject)},
                          true)
                      )
{
    m_out = addOutPort(intelli::typeId<intelli::ObjectData>());
    port(m_out)->captionVisible = false;

    registerWidgetFactory([this]() {
        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(objLinkProp());
        w->setScope(gtApp->currentProject());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };

        connect(this, &Node::evaluated, w.get(), update);

        update();

        return w;
    });

    connect(m_value.get(), &GtAbstractProperty::changed,
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
            connect(object, qOverload<GtObject*,
                    GtAbstractProperty*>(&GtObject::dataChanged),
                    this, &Node::triggerNodeEvaluation, Qt::UniqueConnection);
        }
    });
}

void
ObjectInputNode::eval()
{
    auto* linkedObj = linkedObject();

    revertProperty();

    if (!linkedObj)
    {
        setNodeData(m_out, nullptr);
        return;
    }

    setValue(linkedObj->uuid());

    setNodeData(m_out, std::make_shared<intelli::ObjectData>(linkedObj));
}

GtObject*
ObjectInputNode::linkedObject(GtObject* root)
{
    if (auto prop = objLinkProp()) return prop->linkedObject(root);
    return nullptr;
}

const GtObject*
ObjectInputNode::linkedObject(GtObject* root) const
{
    if (auto prop = objLinkProp()) return prop->linkedObject(root);
    return nullptr;
}

void
ObjectInputNode::setValue(const QString& uuid)
{
    if (auto prop = objLinkProp()) return prop->setVal(uuid);
}

GtObjectLinkProperty*
ObjectInputNode::objLinkProp()
{
    if (m_value.get()) return static_cast<GtObjectLinkProperty*>(m_value.get());

    return nullptr;
}

const
GtObjectLinkProperty *
ObjectInputNode::objLinkProp() const
{
    if (m_value.get()) return static_cast<const GtObjectLinkProperty*>(
                m_value.get());

    return nullptr;
}

void
ObjectInputNode::revertProperty()
{
    if (auto prop = objLinkProp()) return prop->revert();
}
} // namespace intelli
