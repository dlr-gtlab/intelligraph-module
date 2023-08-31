/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/stringlistinput.h"

#include "intelli/data/stringlist.h"

#include <gt_structproperty.h>
#include <gt_stringproperty.h>

#include <QLayout>

using namespace intelli;


StringListInputNode::StringListInputNode() :
    Node(tr("Stringlist Input")),
    m_values("values", "Values")
{
    GtPropertyStructDefinition stringEntryDef{QStringLiteral("StringStruct")};
    stringEntryDef.defineMember(QStringLiteral("value"),
                                gt::makeStringProperty());

    m_values.registerAllowedType(stringEntryDef);

    registerPropertyStructContainer(m_values);

    setNodeFlag(Resizable);
    
    addOutPort(typeId<StringListData>());

    registerWidgetFactory([this]() {
        auto base = makeWidget();
        auto w = new QTextEdit;
        base->layout()->addWidget(w);
        w->setReadOnly(true);
        w->setToolTip(tr("Use the properties dock to add entries."));
        
        connect(this, &Node::evaluated, w, [this, w](){
            w->setPlainText(values().join("\n"));
        });

        return base;
    });

    connect(&m_values, &GtPropertyStructContainer::entryAdded,
            this, &Node::triggerNodeEvaluation);
    connect(&m_values, &GtPropertyStructContainer::entryRemoved,
            this, &Node::triggerNodeEvaluation);
    connect(&m_values, &GtPropertyStructContainer::entryChanged,
            this, &Node::triggerNodeEvaluation);
}

Node::NodeDataPtr
StringListInputNode::eval(PortId outId)
{
    return std::make_shared<StringListData>(values());
}

QStringList
StringListInputNode::values() const
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
