/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/stringselectionnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/stringselection.h>

#include <QComboBox>
#include <QGraphicsWidget>

using namespace intelli;

StringSelectionNodeUI::StringSelectionNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringSelectionNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringSelectionNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<StringSelectionNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<QComboBox>();

        auto const updateOptions = [w_ = w.get()](QStringList const& options) {
            w_->clear();
            w_->addItems(options);
        };
        auto const updateSelection = [w_ = w.get()](QString const& selection) {
            if (selection.isEmpty())
            {
                w_->setCurrentIndex(-1);
                return;
            }
            if (w_->currentText() != selection) w_->setCurrentText(selection);
        };

        QObject::connect(node, &StringSelectionNode::optionsChanged,
                         w.get(), updateOptions);
        QObject::connect(node, &StringSelectionNode::selectionChanged,
                         w.get(), updateSelection);
        QObject::connect(w.get(), &QComboBox::currentTextChanged,
                         w.get(), [node](QString const& text) {
            node->setSelection(text);
        });

        updateOptions(node->options());
        updateSelection(node->selection());

        return convertToGraphicsWidget(std::move(w), object);
    };
}
