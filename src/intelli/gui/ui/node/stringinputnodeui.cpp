/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/stringinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/input/stringinput.h>

#include <gt_lineedit.h>

#include <QGraphicsWidget>

using namespace intelli;

StringInputNodeUI::StringInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<StringInputNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<GtLineEdit>();
        w->setPlaceholderText(QStringLiteral("String"));
        w->setMinimumWidth(50);
        w->resize(100, w->sizeHint().height());

        auto const updateProp = [node, w_ = w.get()]() {
            if (node->value() != w_->text()) node->setValue(w_->text());
        };
        auto const updateText = [node, w_ = w.get()]() {
            if (w_->text() != node->value()) w_->setText(node->value());
        };

        QObject::connect(w.get(), &GtLineEdit::focusOut, w.get(), updateProp);
        QObject::connect(w.get(), &GtLineEdit::clearFocusOut, w.get(), updateProp);
        QObject::connect(node, &StringInputNode::valueChanged, w.get(), updateText);

        updateText();

        return convertToGraphicsWidget(std::move(w), object);
    };
}
