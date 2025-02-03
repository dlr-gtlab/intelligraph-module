/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/stringbuilder.h>

#include <intelli/data/string.h>

#include <gt_lineedit.h>

#include <QLayout>

using namespace intelli;

StringBuilderNode::StringBuilderNode() :
    Node("String Builder"),
    m_pattern("pattern", tr("Pattern"),
              tr("String Builder Pattern"),
              QStringLiteral("%1%2"))
{
    setNodeFlag(ResizableHOnly, true);

    registerProperty(m_pattern);

    m_inA = addInPort({typeId<StringData>(), tr("%1")});
    m_inB = addInPort({typeId<StringData>(), tr("%2")});
    m_out = addOutPort({typeId<StringData>(), tr("new_str")});

    registerWidgetFactory([this]() {
        auto b = makeBaseWidget();
        auto w = new GtLineEdit();
        w->setPlaceholderText(QStringLiteral("%1/%2"));
        w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        w->setMinimumWidth(50);

        b->layout()->addWidget(w);

        auto const updateProp = [this, w](){
            QString const& text = w->text();
            if(m_pattern != text) m_pattern.setVal(text);
        };
        auto const updateText = [this, w = w](){
            QString const& text = m_pattern.get();
            if (w->text() != text) w->setText(text);
        };

        connect(w, &GtLineEdit::focusOut, this, updateProp);
        connect(w, &GtLineEdit::clearFocusOut, this, updateProp);
        connect(&m_pattern, &GtAbstractProperty::changed, w, updateText);

        updateText();

        return b;
    });

    connect(&m_pattern, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
StringBuilderNode::eval()
{
    QString a, b;

    if (auto dataA = nodeData<StringData>(m_inA)) a = dataA->value();
    if (auto dataB = nodeData<StringData>(m_inB)) b = dataB->value();

    QString result = m_pattern.get();

    if (result.contains("%1")) result = result.arg(a);
    if (result.contains("%2")) result = result.arg(b);

    setNodeData(m_out, std::make_shared<StringData>(std::move(result)));
}
