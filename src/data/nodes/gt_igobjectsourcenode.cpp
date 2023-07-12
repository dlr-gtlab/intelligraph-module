#include "gt_igobjectsourcenode.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_objectfactory.h"
#include "gt_application.h"
#include "gt_project.h"

#include "gt_igobjectdata.h"
#include "gt_igstringlistdata.h"

GTIG_REGISTER_NODE(GtIgObjectSourceNode, "Object");


GtIgObjectSourceNode::GtIgObjectSourceNode() :
    GtIntelliGraphNode(tr("Object Source")),
    m_object("target", tr("Target"), tr("Target Object"),
             this, gtObjectFactory->knownClasses())
{
    registerProperty(m_object);

    m_in  = addInPort(gt::ig::typeId<GtIgStringListData>());
    m_out = addOutPort(gt::ig::typeId<GtIgObjectData>());

    registerWidgetFactory([this](GtIntelliGraphNode&) {
        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&m_object);
        w->setScope(gtApp->currentProject());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };

        connect(this, &GtIntelliGraphNode::outDataUpdated, w.get(), update);
        connect(this, &GtIntelliGraphNode::outDataInvalidated, w.get(), update);

        update();

        return w;
    });

    connect(&m_object, &GtAbstractProperty::changed,
            this, &GtIgObjectSourceNode::updateNode);
}

GtIntelliGraphNode::NodeData
GtIgObjectSourceNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto* linkedObject = m_object.linkedObject();

    if (linkedObject) linkedObject->disconnect(this);

    m_object.revert();

    auto* filterData = nodeData<GtIgStringListData*>(m_in);
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
            this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);
    connect(linkedObject, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
            this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);

    return std::make_shared<GtIgObjectData>(linkedObject);
}
