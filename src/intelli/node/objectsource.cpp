#include "intelli/node/objectsource.h"
#include "intelli/nodefactory.h"

#include "gt_objectfactory.h"
#include "gt_application.h"
#include "gt_project.h"

#include "intelli/data/object.h"
#include "intelli/data/stringlist.h"
#include "gt_propertyobjectlinkeditor.h"

using namespace intelli;

GTIG_REGISTER_NODE(ObjectSourceNode, "Object");

ObjectSourceNode::ObjectSourceNode() :
    Node(tr("Object Source")),
    m_object("target", tr("Target"), tr("Target Object"),
             gtApp->currentProject(), gtObjectFactory->knownClasses())
{
    registerProperty(m_object);
    
    m_in  = addInPort(typeId<StringListData>());
    m_out = addOutPort(typeId<ObjectData>());

    registerWidgetFactory([this]() {
        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&m_object);
        w->setScope(gtApp->currentProject());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };
        
        connect(this, &Node::outDataUpdated, w.get(), update);
        connect(this, &Node::outDataInvalidated, w.get(), update);

        update();

        return w;
    });

    connect(&m_object, &GtAbstractProperty::changed,
            this, &ObjectSourceNode::updateNode);
}

Node::NodeDataPtr
ObjectSourceNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto* linkedObject = m_object.linkedObject();

    if (linkedObject) linkedObject->disconnect(this);

    m_object.revert();
    
    auto* filterData = nodeData<StringListData*>(m_in);
    if (filterData)
    {
        m_object.setAllowedClasses(filterData->value());
    }

    if (!linkedObject || !m_object.allowedClasses().contains(linkedObject->metaObject()->className()))
    {
        return {};
    }

    m_object.setVal(linkedObject->uuid());

    connect(linkedObject, qOverload<GtObject*>(&GtObject::dataChanged),
            this, &ObjectSourceNode::updateNode, Qt::UniqueConnection);
    connect(linkedObject, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
            this, &ObjectSourceNode::updateNode, Qt::UniqueConnection);
    
    return std::make_shared<ObjectData>(linkedObject);
}
