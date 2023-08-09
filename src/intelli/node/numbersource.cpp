/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/numbersource.h"

#include "intelli/nodefactory.h"
#include "intelli/data/double.h"

#include "gt_regexp.h"
#include "gt_lineedit.h"

#include <QRegExpValidator>

using namespace intelli;

GT_INTELLI_REGISTER_NODE(NumberSourceNode, "Number");

NumberSourceNode::NumberSourceNode() :
    Node("Number Source"),
    m_value("value", tr("Value"), tr("Value"))
{
    registerProperty(m_value);

    m_out = addOutPort(typeId<DoubleData>());

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::updateNode);

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
        connect(this, &Node::outDataUpdated, w.get(), updateText);

        updateText();

        return w;
    });
}

Node::NodeDataPtr
NumberSourceNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    return std::make_shared<DoubleData>(m_value);
}

