/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/numberdisplaynodeui.h>

#include <intelli/node/numberdisplay.h>
#include <intelli/gui/graphics/nodeobject.h>

#include <gt_lineedit.h>

#include <QGraphicsWidget>

using namespace intelli;

NumberDisplayNodeUI::NumberDisplayNodeUI() = default;

NodeUI::WidgetFactoryFunction
NumberDisplayNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<NumberDisplayNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<NumberDisplayNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<GtLineEdit>();
        w->setReadOnly(true);
        w->setMinimumWidth(75);
        w->resize(w->minimumSizeHint());

        auto const updateText = [node, w_ = w.get()]() {
            w_->setText(QString::number(node->displayValue()));
        };

        QObject::connect(node, &Node::evaluated, w.get(), updateText);
        updateText();

        return convertToGraphicsWidget(std::move(w), object);
    };
}
