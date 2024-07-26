/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "stringinputnode.h"

#include <intelli/data/string.h>

#include <gt_lineedit.h>

namespace intelli
{
StringInputNode::StringInputNode() :
    AbstractInputNode("String Input",
                      std::make_unique<GtStringProperty>("value", tr("Value"),
                                                         tr("Current value")))
{
    setNodeFlag(ResizableHOnly, true);

    m_value->hide();

    m_out = addOutPort(intelli::typeId<intelli::StringData>());
    port(m_out)->captionVisible = false;

    connect(m_value.get(), &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    registerWidgetFactory([this]() {
        auto w = std::make_unique<GtLineEdit>();
        w->setPlaceholderText(QStringLiteral("String"));
        w->setMinimumWidth(50);

        auto const updateProp = [this, w_ = w.get()](){
            if(value() != w_->text()) setValue(w_->text());
        };
        auto const updateText = [this, w_ = w.get()](){
            if (w_->text() != value()) w_->setText(value());
        };

        connect(w.get(), &GtLineEdit::focusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::clearFocusOut, this, updateProp);
        connect(m_value.get(), &GtAbstractProperty::changed,
                w.get(), updateText);

        updateText();

        return w;
    });
}

QString
StringInputNode::value() const
{
    auto prop = static_cast<GtStringProperty*>(m_value.get());
    return prop->getVal();
}

void
StringInputNode::setValue(const QString &value)
{
    auto prop = static_cast<GtStringProperty*>(m_value.get());
    prop->setVal(value);
}

void
StringInputNode::eval()
{
    setNodeData(m_out, std::make_shared<intelli::StringData>(value()));
}
} // namespace intelli
