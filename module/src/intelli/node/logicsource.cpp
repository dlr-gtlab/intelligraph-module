/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/logicsource.h"
#include "intelli/gui/property_item/logicdisplay.h"

#include <intelli/data/bool.h>

#include <QLayout>

using namespace intelli;

LogicSourceNode::LogicSourceNode() :
    Node(QStringLiteral("Logic Source")),
    m_value(QStringLiteral("value"), QStringLiteral("Value"), QStringLiteral("value"), false)
{
    registerProperty(m_value);

    setNodeEvalMode(NodeEvalMode::MainThread);

    m_out = addOutPort(typeId<BoolData>());

    registerWidgetFactory([this](){
        auto base = makeBaseWidget();

        auto* w = new LogicDisplayWidget(m_value);
        base->layout()->addWidget(w);

        connect(w, &LogicDisplayWidget::valueChanged, this, [this](bool newVal){
            if (m_value != newVal) m_value = newVal;
        });

        connect(&m_value, &GtAbstractProperty::changed, w, [this, w](){
            w->setValue(m_value);
        });

        return base;
    });

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
LogicSourceNode::eval()
{
    setNodeData(m_out, std::make_shared<BoolData>(m_value));
}

