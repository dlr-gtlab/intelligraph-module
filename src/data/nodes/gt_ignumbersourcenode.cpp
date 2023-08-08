/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_ignumbersourcenode.h"

#include "gt_intelligraphnodefactory.h"
#include "gt_igdoubledata.h"

#include "gt_regexp.h"
#include "gt_lineedit.h"

#include <QRegExpValidator>

GTIG_REGISTER_NODE(GtIgNumberSourceNode, "Number");

GtIgNumberSourceNode::GtIgNumberSourceNode() :
    GtIntelliGraphNode("Number Source"),
    m_value("value", tr("Value"), tr("Value"))
{
    registerProperty(m_value);

    m_out = addOutPort(gt::ig::typeId<GtIgDoubleData>());

    connect(&m_value, &GtAbstractProperty::changed,
            this, &GtIntelliGraphNode::updateNode);

    registerWidgetFactory([=](){
        auto w = std::make_unique<GtLineEdit>();
        w->setFixedWidth(50);
        w->setValidator(new QRegExpValidator(gt::re::forDoubles()));

        // react to user inputs
        auto const updateProp = [this, w_ = w.get()](){
            m_value = w_->text().toDouble();
        };
        connect(w.get(), &GtLineEdit::focusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::clearFocusOut, this, updateProp);

        // react to external changes
        auto const updateText = [this, w_ = w.get()](){
            w_->setText(QString::number(m_value.get()));
        };
        connect(this, &GtIntelliGraphNode::outDataUpdated, w.get(), updateText);

        updateText();

        return w;
    });
}

GtIntelliGraphNode::NodeData
GtIgNumberSourceNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    return std::make_shared<GtIgDoubleData>(m_value);
}

