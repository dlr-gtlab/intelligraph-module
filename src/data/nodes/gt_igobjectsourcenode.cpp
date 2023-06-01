#include "gt_igobjectsourcenode.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_objectfactory.h"
#include "gt_application.h"
#include "gt_project.h"

#include "models/data/gt_igobjectdata.h"
#include "models/data/gt_igstringlistdata.h"

#include <QEvent>

GTIG_REGISTER_NODE(GtIgObjectSourceNode, "Object");

GtIgObjectSourceNode::GtIgObjectSourceNode() :
    GtIntelliGraphNode(tr("Object Source")),
    m_object("target", tr("Target"), tr("Target Object"),
             this, gtObjectFactory->knownClasses())
{
    registerProperty(m_object);

    connect(&m_object, &GtAbstractProperty::changed,
            this, &GtIgObjectSourceNode::updateNode);
}

unsigned int
GtIgObjectSourceNode::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
    case PortType::Out:
        return 1;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgObjectSourceNode::NodeDataType
GtIgObjectSourceNode::dataType(PortType const type,
                                PortIndex const idx) const
{
    switch (type)
    {
    case PortType::In:
        return GtIgStringListData::staticType();
    case PortType::Out:
        return GtIgObjectData::staticType();
    case PortType::None:
        return {};
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgObjectSourceNode::NodeData
GtIgObjectSourceNode::outData(PortIndex)
{
    return std::make_shared<GtIgObjectData>(m_object.linkedObject());
}

void
GtIgObjectSourceNode::setInData(NodeData data, const QtNodes::PortIndex port)
{
    auto filterData = gt::ig::nodedata_cast<GtIgStringListData>(std::move(data));

    m_object.setAllowedClasses(filterData ? filterData->values() :
                                            gtObjectFactory->knownClasses());

    // current obj may no longer be "allowed"
    if (auto* obj = m_object.linkedObject())
    {
        if (!m_object.allowedClasses().contains(obj->metaObject()->className()))
        {
            m_object.revert();
            updateNode();
        }
    }
}

QWidget*
GtIgObjectSourceNode::embeddedWidget()
{
    if (!m_editor) initWidget();
    return m_editor;
}

void
GtIgObjectSourceNode::initWidget()
{
    m_editor = gt::ig::make_volatile<GtPropertyObjectLinkEditor>();
    m_editor->setObjectLinkProperty(&m_object);
    m_editor->setScope(gtApp->currentProject());
}

void
GtIgObjectSourceNode::updateNode()
{
    if (m_editor) m_editor->updateText();

    if (auto* linkedObject = m_object.linkedObject())
    {
        if (m_lastObject && m_lastObject != linkedObject)
        {
            m_lastObject->disconnect(this);
        }

        m_lastObject = linkedObject;

        connect(linkedObject, qOverload<GtObject*>(&GtObject::dataChanged),
                this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);
        connect(linkedObject, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                this, &GtIgObjectSourceNode::updateNode, Qt::UniqueConnection);

        emit dataUpdated(0);
        return;
    }

    emit dataInvalidated(0);
}
