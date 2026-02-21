/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/numbermathnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/utilities.h>
#include <intelli/node/numbermath.h>

#include <QComboBox>
#include <QGraphicsWidget>
#include <QLayout>

using namespace intelli;

NumberMathNodeUI::NumberMathNodeUI() = default;

NodeUI::WidgetFactoryFunction
NumberMathNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<NumberMathNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<NumberMathNode*>(&source);
        if (!node) return nullptr;

        auto base = utils::makeWidgetWithLayout();
        auto* combo = new QComboBox();
        base->layout()->addWidget(combo);
        combo->addItems(QStringList{"+", "-", "*", "/", "pow"});

        auto const toText = [](NumberMathNode::MathOperation op) {
            switch (op)
            {
            case NumberMathNode::Minus:
                return QStringLiteral("-");
            case NumberMathNode::Divide:
                return QStringLiteral("/");
            case NumberMathNode::Multiply:
                return QStringLiteral("*");
            case NumberMathNode::Power:
                return QStringLiteral("pow");
            case NumberMathNode::Plus:
                break;
            }
            return QStringLiteral("+");
        };

        auto const fromText = [](QString const& text) {
            if (text == QStringLiteral("-")) return NumberMathNode::Minus;
            if (text == QStringLiteral("*")) return NumberMathNode::Multiply;
            if (text == QStringLiteral("/")) return NumberMathNode::Divide;
            if (text == QStringLiteral("pow")) return NumberMathNode::Power;
            return NumberMathNode::Plus;
        };

        auto const update = [node, combo, toText]() {
            combo->setCurrentText(toText(node->operation()));
        };

        QObject::connect(node, &NumberMathNode::operationChanged,
                         combo, update);

        QObject::connect(combo, &QComboBox::currentTextChanged,
                         combo, [node, fromText](QString const& text) {
            auto next = fromText(text);
            if (next == node->operation()) return;
            node->setOperation(next);
        });

        update();

        return convertToGraphicsWidget(std::move(base), object);
    };
}
