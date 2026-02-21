/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/stringbuildernodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/utilities.h>
#include <intelli/node/stringbuilder.h>

#include <gt_lineedit.h>

#include <QGraphicsWidget>
#include <QLayout>

using namespace intelli;

StringBuilderNodeUI::StringBuilderNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringBuilderNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringBuilderNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<StringBuilderNode*>(&source);
        if (!node) return nullptr;

        auto base = utils::makeWidgetWithLayout();
        auto* lay = base->layout();

        auto* edit = new GtLineEdit();
        edit->setPlaceholderText(QStringLiteral("%1/%2"));
        edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        edit->setMinimumWidth(50);
        lay->addWidget(edit);

        auto const updatePattern = [node, edit]() {
            QString const& text = edit->text();
            if (node->pattern() != text) node->setPattern(text);
        };
        auto const updateText = [node, edit]() {
            QString const& text = node->pattern();
            if (edit->text() != text) edit->setText(text);
        };

        QObject::connect(edit, &GtLineEdit::focusOut, edit, updatePattern);
        QObject::connect(edit, &GtLineEdit::clearFocusOut, edit, updatePattern);
        QObject::connect(node, &StringBuilderNode::patternChanged, edit, updateText);

        updateText();

        return convertToGraphicsWidget(std::move(base), object);
    };
}
