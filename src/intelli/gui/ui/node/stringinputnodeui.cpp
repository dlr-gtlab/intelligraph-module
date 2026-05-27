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
#include <QVBoxLayout>

using namespace intelli;

StringInputNodeWidget::StringInputNodeWidget(StringInputNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_edit = new GtLineEdit(this);
    m_edit->setPlaceholderText(QStringLiteral("String"));
    m_edit->setMinimumWidth(50);
    m_edit->resize(100, m_edit->sizeHint().height());
    lay->addWidget(m_edit);

    QObject::connect(m_edit, &GtLineEdit::focusOut,
                     this, &StringInputNodeWidget::updateNodeFromText);
    QObject::connect(m_edit, &GtLineEdit::clearFocusOut,
                     this, &StringInputNodeWidget::updateNodeFromText);
    QObject::connect(m_node, &StringInputNode::valueChanged,
                     this, &StringInputNodeWidget::updateTextFromNode);

    updateTextFromNode();
}

NodeUI::QGraphicsWidgetPtr
StringInputNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<StringInputNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<StringInputNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
StringInputNodeWidget::updateNodeFromText()
{
    if (m_node->value() != m_edit->text()) m_node->setValue(m_edit->text());
}

void
StringInputNodeWidget::updateTextFromNode()
{
    if (m_edit->text() != m_node->value()) m_edit->setText(m_node->value());
}

StringInputNodeUI::StringInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringInputNode const*>(&n)) return {};

    return &StringInputNodeWidget::create;
}
