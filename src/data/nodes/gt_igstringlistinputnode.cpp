/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_igstringlistinputnode.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_igstringlistdata.h"

#include "gt_structproperty.h"
#include "gt_stringproperty.h"

GTIG_REGISTER_NODE(GtIgStringListInputNode, "Input")

GtIgStringListInputNode::GtIgStringListInputNode() :
    GtIntelliGraphNode(tr("Stringlist Input")),
    m_values("values", "Values")
{

    GtPropertyStructDefinition stringEntryDef{QStringLiteral("StringStruct")};
    stringEntryDef.defineMember(QStringLiteral("value"),
                                gt::makeStringProperty());

    m_values.registerAllowedType(stringEntryDef);

    registerPropertyStructContainer(m_values);

    setNodeFlag(gt::ig::Resizable);

    addOutPort(gt::ig::typeId<GtIgStringListData>());

    registerWidgetFactory([this]() {
        auto w = std::make_unique<QTextEdit>();
        w->setReadOnly(true);
        w->setToolTip(tr("Use the properties dock to add entries."));
        
        connect(this, &GtIntelliGraphNode::outDataUpdated,
                w.get(), [this, w_ = w.get()](){
            w_->setPlainText(values().join("\n"));
        });

        return w;
    });

    connect(&m_values, &GtPropertyStructContainer::entryAdded,
            this, &GtIntelliGraphNode::updateNode);
    connect(&m_values, &GtPropertyStructContainer::entryRemoved,
            this, &GtIntelliGraphNode::updateNode);
    connect(&m_values, &GtPropertyStructContainer::entryChanged,
            this, &GtIntelliGraphNode::updateNode);
}

GtIntelliGraphNode::NodeData
GtIgStringListInputNode::eval(PortId outId)
{
    return std::make_shared<GtIgStringListData>(values());
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
