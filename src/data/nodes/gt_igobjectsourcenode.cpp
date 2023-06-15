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
             this, QStringList{})
{
    registerProperty(m_object);

    m_inPort  = addInPort(gt::ig::typeId<GtIgStringListData>(), Required);
    addOutPort(gt::ig::typeId<GtIgObjectData>());

    registerWidgetFactory([this](GtIntelliGraphNode&) {
        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&m_object);
        w->setScope(gtApp->currentProject());

        auto update = [w_ = w.get()](){
            w_->updateText();
        };

        connect(this, &GtIntelliGraphNode::outDataUpdated, w.get(), update);
        connect(this, &GtIntelliGraphNode::outDataInvalidated, w.get(), update);

        return w;
    });

    connect(&m_object, &GtAbstractProperty::changed,
            this, &GtIgObjectSourceNode::updateNode);
}

GtIntelliGraphNode::NodeData
GtIgObjectSourceNode::eval(PortId outId)
{
    auto* linkedObject = m_object.linkedObject();

    if (linkedObject) linkedObject->disconnect(this);

    m_object.revert();

    auto* filterData = qobject_cast<GtIgStringListData const*>(portData(m_inPort));
    if (!filterData)
    {
        gtDebug() << "HERE 1" << linkedObject << m_object.allowedClasses();
        return {};
    }

    m_object.setAllowedClasses(filterData->values());

    if (!linkedObject || !m_object.allowedClasses().contains(linkedObject->metaObject()->className()))
    {
        gtDebug() << "HERE 2" << linkedObject << m_object.allowedClasses();
        return {};
    }

    m_object.setVal(linkedObject->uuid());

    connect(linkedObject, qOverload<GtObject*>(&GtObject::dataChanged),
            this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);
    connect(linkedObject, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
            this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);

    gtDebug() << "HERE 3" << linkedObject;

    return std::make_shared<GtIgObjectData>(linkedObject);
}
