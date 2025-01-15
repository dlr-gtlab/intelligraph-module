/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "stringinputnode.h"

#include <intelli/data/string.h>

#include <gt_lineedit.h>

using namespace intelli;

StringInputNode::StringInputNode() :
    Node("String Input"),
    m_value("value", tr("Value"), tr("Current value"))
{
    registerProperty(m_value);

    setNodeFlag(ResizableHOnly, true);
    setNodeEvalMode(NodeEvalMode::Blocking);

    m_out = addOutPort(makePort(typeId<StringData>())
                           .setCaptionVisible(false));

    connect(&m_value, &GtAbstractProperty::changed,
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
        connect(&m_value, &GtAbstractProperty::changed, w.get(), updateText);

        updateText();

        return w;
    });
}

QString const&
StringInputNode::value() const
{
    return m_value.get();
}

void
StringInputNode::setValue(QString value)
{
    m_value = std::move(value);
}

void
StringInputNode::eval()
{
    setNodeData(m_out, std::make_shared<intelli::StringData>(value()));
}
