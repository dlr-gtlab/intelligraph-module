/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/doubleinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/widgets/doubleinputwidget.h>
#include <intelli/node/input/doubleinput.h>

#include <QGraphicsWidget>

using namespace intelli;

DoubleInputNodeUI::DoubleInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
DoubleInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<DoubleInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<DoubleInputNode*>(&source);
        if (!node) return nullptr;

        using InputMode = DoubleInputWidget::InputMode;

        auto* w = new DoubleInputWidget(static_cast<InputMode>(node->inputModeValue()));

        auto updateRange = [node, w]() {
            w->setRange(node->value(), node->lowerBound(), node->upperBound());
        };

        auto updateMode = [node, w]() {
            w->setInputMode(static_cast<InputMode>(node->inputModeValue()));
            node->setUseBounds(w->useBounds());
        };

        QObject::connect(w, &DoubleInputWidget::valueComitted,
                         w, [node, w]() {
            double newVal = w->value();
            if (node->value() != newVal) node->setValue(newVal);
        });
        QObject::connect(w, &DoubleInputWidget::minChanged,
                         w, [node, w]() {
            double newVal = w->min();
            if (node->lowerBound() != newVal) node->setLowerBound(newVal);
        });
        QObject::connect(w, &DoubleInputWidget::maxChanged,
                         w, [node, w]() {
            double newVal = w->max();
            if (node->upperBound() != newVal) node->setUpperBound(newVal);
        });

        QObject::connect(node, &DoubleInputNode::rangeChanged,
                         w, updateRange);
        QObject::connect(node, &DoubleInputNode::inputModeChanged,
                         w, updateMode);

        updateRange();
        updateMode();

        return convertToGraphicsWidget(std::unique_ptr<QWidget>(w), object);
    };
}
