/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igfinddirectchildnode.h"
#include "gt_intelligraphnodefactory.h"

#include "models/data/gt_igobjectdata.h"

#include <QRegExpValidator>

GTIG_REGISTER_NODE(GtIgFindDirectChildNode, "Object")

GtIgFindDirectChildNode::GtIgFindDirectChildNode() :
    GtIntelliGraphNode(tr("Find Direct Child")),
    m_childClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child"))
{
    registerProperty(m_childClassName);

    connect(&m_childClassName, &GtAbstractProperty::changed,
            this, &GtIgFindDirectChildNode::updateNode);
}

unsigned
GtIgFindDirectChildNode::nPorts(const PortType type) const
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

GtIntelliGraphNode::NodeDataType
GtIgFindDirectChildNode::dataType(const PortType type, const PortIndex idx) const
{
    switch (type)
    {
    case PortType::In:
    case PortType::Out:
        return GtIgObjectData::staticType();
    case PortType::None:
        return {};
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIntelliGraphNode::NodeData
GtIgFindDirectChildNode::outData(const PortIndex port)
{
    if (!m_parent || !m_parent->object()) return {};

    auto const children = m_parent->object()->findDirectChildren();
    auto iter = std::find_if(std::begin(children), std::end(children),
                             [className = m_childClassName.get()](GtObject* c){
        return className == c->metaObject()->className();
    });

    if (iter == std::end(children)) return {};

    return std::make_shared<GtIgObjectData>(*iter);
}

void
GtIgFindDirectChildNode::setInData(NodeData data, const PortIndex port)
{
    m_parent = gt::ig::nodedata_cast<GtIgObjectData>(std::move(data));

    updateNode();
}

QWidget*
GtIgFindDirectChildNode::embeddedWidget()
{
    if (!m_editor) initWidget();
    return m_editor;
}

void
GtIgFindDirectChildNode::updateNode()
{
    if (m_editor) m_editor->setText(m_childClassName);

    if (m_parent || !m_childClassName.get().isEmpty())
    {
        emit dataUpdated(0);
    }
    else
    {
        emit dataInvalidated(0);
    }
}

void
GtIgFindDirectChildNode::initWidget()
{
    m_editor = gt::ig::make_volatile<GtLineEdit>();
    m_editor->setValidator(new QRegExpValidator(gt::re::ig::forClassNames()));

    auto const updateProp = [this](){
        m_childClassName = m_editor->text();
    };

    connect(m_editor, &GtLineEdit::focusOut, this, updateProp);
    connect(m_editor, &GtLineEdit::clearFocusOut, this, updateProp);
}

