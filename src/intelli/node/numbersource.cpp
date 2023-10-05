/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/numbersource.h"

#include "intelli/data/double.h"

#include "gt_regexp.h"
#include "gt_lineedit.h"

#include <QRegExpValidator>

using namespace intelli;


NumberSourceNode::NumberSourceNode() :
    Node("Number Source"),
    m_value("value", tr("Value"), tr("Value"))
{
    registerProperty(m_value);

    m_out = addOutPort(typeId<DoubleData>());

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    registerWidgetFactory([=](){
        auto w = std::make_unique<GtLineEdit>();
        w->setFixedWidth(50);
        w->setValidator(new QRegExpValidator(gt::re::forDoubles()));


        // react to user inputs
        auto const updateProp = [this, w_ = w.get()](){
            double tmp = w_->text().toDouble();
            if (m_value != tmp) m_value = tmp;
        };
        connect(w.get(), &GtLineEdit::focusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::clearFocusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::returnPressed, this, updateProp);

        // react to external changes
        auto const updateText = [this, w_ = w.get()](){
            w_->setText(QString::number(m_value.get()));
        };
        connect(this, &Node::evaluated, w.get(), updateText);

        updateText();

        return w;
    });
}

void
NumberSourceNode::eval()
{
    setNodeData(m_out, std::make_shared<DoubleData>(m_value));
}

