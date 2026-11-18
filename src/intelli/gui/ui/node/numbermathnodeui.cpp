/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/numbermathnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/numbermath.h>

#include <QComboBox>
#include <QGraphicsWidget>
#include <QVBoxLayout>

using namespace intelli;

NumberMathNodeWidget::NumberMathNodeWidget(NumberMathNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_combo = new QComboBox(this);
    lay->addWidget(m_combo);
    m_combo->addItems(QStringList{"+", "-", "*", "/", "pow"});

    QObject::connect(m_node, &NumberMathNode::operationChanged,
                     this, &NumberMathNodeWidget::updateComboFromNode);
    QObject::connect(m_combo, &QComboBox::currentTextChanged,
                     this, &NumberMathNodeWidget::updateNodeFromCombo);

    updateComboFromNode();
}

NodeUI::QGraphicsWidgetPtr
NumberMathNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<NumberMathNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<NumberMathNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

QString
NumberMathNodeWidget::toText(NumberMathNode::MathOperation op)
{
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
}

NumberMathNode::MathOperation
NumberMathNodeWidget::fromText(QString const& text)
{
    if (text == QStringLiteral("-")) return NumberMathNode::Minus;
    if (text == QStringLiteral("*")) return NumberMathNode::Multiply;
    if (text == QStringLiteral("/")) return NumberMathNode::Divide;
    if (text == QStringLiteral("pow")) return NumberMathNode::Power;
    return NumberMathNode::Plus;
}

void
NumberMathNodeWidget::updateComboFromNode()
{
    m_combo->setCurrentText(toText(m_node->operation()));
}

void
NumberMathNodeWidget::updateNodeFromCombo(QString const& text)
{
    auto next = fromText(text);
    if (next == m_node->operation()) return;
    m_node->setOperation(next);
}

NumberMathNodeUI::NumberMathNodeUI() = default;

NodeUI::WidgetFactoryFunction
NumberMathNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<NumberMathNode const*>(&n)) return {};

    return &NumberMathNodeWidget::create;
}
