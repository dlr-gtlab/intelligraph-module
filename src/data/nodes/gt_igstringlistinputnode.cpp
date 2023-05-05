/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_igstringlistinputnode.h"

#include "models/data/gt_igstringlistdata.h"

#include "gt_structproperty.h"
#include "gt_stringproperty.h"

GTIG_REGISTER_NODE(GtIgStringListInputNode)

GtIgStringListInputNode::GtIgStringListInputNode() :
    GtIntelliGraphNode(tr("Stringlist Input")),
    m_values("values", "Values")
{
    setNodeFlag(gt::ig::Resizable);

    GtPropertyStructDefinition stringEntryDef(QStringLiteral("StringStruct"));
    stringEntryDef.defineMember(QStringLiteral("value"),
                                gt::makeStringProperty());

    m_values.registerAllowedType(stringEntryDef);

    registerPropertyStructContainer(m_values);

    connect(&m_values, &GtPropertyStructContainer::entryAdded,
            this, &GtIntelliGraphNode::updateNode);
    connect(&m_values, &GtPropertyStructContainer::entryRemoved,
            this, &GtIntelliGraphNode::updateNode);
    connect(&m_values, &GtPropertyStructContainer::entryChanged,
            this, &GtIntelliGraphNode::updateNode);
}

unsigned int
GtIgStringListInputNode::nPorts(const PortType type) const
{
    switch (type)
    {
    case PortType::Out:
        return 1;
    case PortType::In:
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgStringListInputNode::NodeDataType
GtIgStringListInputNode::dataType(const PortType type, const PortIndex idx) const
{
    return GtIgStringListData::staticType();
}

GtIgStringListInputNode::NodeData
GtIgStringListInputNode::outData(const PortIndex port)
{
    return std::make_shared<GtIgStringListData>(values());
}

QWidget*
GtIgStringListInputNode::embeddedWidget()
{
    if (!m_editor) initWidget();
    return m_editor;
}

void
GtIgStringListInputNode::initWidget()
{
    m_editor = gt::ig::make_volatile<QTextEdit>();
    m_editor->setReadOnly(true);
    m_editor->setToolTip(tr("Use the properties dock to add entries."));
}

QStringList
GtIgStringListInputNode::values() const
{
    QStringList data;
    data.reserve(m_values.size());
    for (auto const& entry : m_values)
    {
        bool ok = true;
        auto value = entry.getMemberVal<QString>(QStringLiteral("value"), &ok);
        if (ok) data << value;
    }
    return data;
}

void
GtIgStringListInputNode::updateNode()
{
    if (m_editor) m_editor->setPlainText(values().join("\n"));

    emit dataUpdated(0);
}
