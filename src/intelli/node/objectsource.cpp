#include "intelli/node/objectsource.h"
#include "intelli/nodefactory.h"

#include "gt_objectfactory.h"
#include "gt_application.h"
#include "gt_project.h"

#include "intelli/data/object.h"
#include "intelli/data/stringlist.h"
#include "gt_propertyobjectlinkeditor.h"

using namespace intelli;

static auto init_once = [](){
    return GT_INTELLI_REGISTER_NODE(ObjectSourceNode, "Object")
}();

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
                    this, &Node::updateNode, Qt::UniqueConnection);
            connect(object, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                    this, &Node::updateNode, Qt::UniqueConnection);
        }
    });
}

Node::NodeDataPtr
ObjectSourceNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto* linkedObject = m_object.linkedObject();

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
    
    return std::make_shared<ObjectData>(linkedObject);
}
