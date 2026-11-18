/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/stringbuildernodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/stringbuilder.h>

#include <gt_lineedit.h>

#include <QGraphicsWidget>
#include <QVBoxLayout>

using namespace intelli;

StringBuilderNodeWidget::StringBuilderNodeWidget(StringBuilderNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_edit = new GtLineEdit(this);
    m_edit->setPlaceholderText(QStringLiteral("%1/%2"));
    m_edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_edit->setMinimumWidth(50);
    lay->addWidget(m_edit);

    QObject::connect(m_edit, &GtLineEdit::focusOut,
                     this, &StringBuilderNodeWidget::updateNodeFromPattern);
    QObject::connect(m_edit, &GtLineEdit::clearFocusOut,
                     this, &StringBuilderNodeWidget::updateNodeFromPattern);
    QObject::connect(m_node, &StringBuilderNode::patternChanged,
                     this, &StringBuilderNodeWidget::updatePatternFromNode);

    updatePatternFromNode();
}

NodeUI::QGraphicsWidgetPtr
StringBuilderNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<StringBuilderNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<StringBuilderNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
StringBuilderNodeWidget::updatePatternFromNode()
{
    QString const& text = m_node->pattern();
    if (m_edit->text() != text) m_edit->setText(text);
}

void
StringBuilderNodeWidget::updateNodeFromPattern()
{
    QString const& text = m_edit->text();
    if (m_node->pattern() != text) m_node->setPattern(text);
}

StringBuilderNodeUI::StringBuilderNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringBuilderNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringBuilderNode const*>(&n)) return {};

    return &StringBuilderNodeWidget::create;
}
