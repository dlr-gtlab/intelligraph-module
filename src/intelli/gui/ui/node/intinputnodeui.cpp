/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/intinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/widgets/intinputwidget.h>
#include <intelli/node/input/intinput.h>

#include <QGraphicsWidget>

using namespace intelli;

IntInputNodeUI::IntInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
IntInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<IntInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<IntInputNode*>(&source);
        if (!node) return nullptr;

        using InputMode = IntInputWidget::InputMode;

        auto* w = new IntInputWidget(static_cast<InputMode>(node->inputModeValue()));

        auto updateRange = [node, w]() {
            w->setRange(node->value(), node->lowerBound(), node->upperBound());
        };

        auto updateMode = [node, w]() {
            w->setInputMode(static_cast<InputMode>(node->inputModeValue()));
            node->setUseBounds(w->useBounds());
        };

        QObject::connect(w, &IntInputWidget::valueComitted,
                         w, [node, w]() {
            int newVal = w->value();
            if (node->value() != newVal) node->setValue(newVal);
        });
        QObject::connect(w, &IntInputWidget::minChanged,
                         w, [node, w]() {
            int newVal = w->min();
            if (node->lowerBound() != newVal) node->setLowerBound(newVal);
        });
        QObject::connect(w, &IntInputWidget::maxChanged,
                         w, [node, w]() {
            int newVal = w->max();
            if (node->upperBound() != newVal) node->setUpperBound(newVal);
        });

        QObject::connect(node, &IntInputNode::rangeChanged,
                         w, updateRange);
        QObject::connect(node, &IntInputNode::inputModeChanged,
                         w, updateMode);

        updateRange();
        updateMode();

        return convertToGraphicsWidget(std::unique_ptr<QWidget>(w), object);
    };
}
