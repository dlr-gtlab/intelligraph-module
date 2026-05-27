/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/stringselectionnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/stringselection.h>
#include <intelli/nodedatainterface.h>

#include <QGraphicsWidget>

using namespace intelli;

StringSelectionNodeWidget::StringSelectionNodeWidget(StringSelectionNode& node) :
    QComboBox()
{
    m_node = &node;

    QObject::connect(m_node, &StringSelectionNode::optionsChanged,
                     this, &StringSelectionNodeWidget::updateOptionsFromNode);
    QObject::connect(m_node, &StringSelectionNode::selectionChanged,
                     this, &StringSelectionNodeWidget::updateSelectionFromNode);
    QObject::connect(this, &QComboBox::currentTextChanged,
                     this, &StringSelectionNodeWidget::updateNodeFromSelection);

    if (exec::nodeDataInterface(*m_node))
    {
        updateOptionsFromNode();
    }
    updateSelectionFromNode();
}

NodeUI::QGraphicsWidgetPtr
StringSelectionNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<StringSelectionNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<StringSelectionNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
StringSelectionNodeWidget::updateOptionsFromNode()
{
    clear();
    addItems(m_node->options());
}

void
StringSelectionNodeWidget::updateSelectionFromNode()
{
    auto const selection = m_node->selection();
    if (selection.isEmpty())
    {
        setCurrentIndex(-1);
        return;
    }

    if (currentText() != selection) setCurrentText(selection);
}

void
StringSelectionNodeWidget::updateNodeFromSelection(QString const& text)
{
    m_node->setSelection(text);
}

StringSelectionNodeUI::StringSelectionNodeUI() = default;

NodeUI::WidgetFactoryFunction
StringSelectionNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<StringSelectionNode const*>(&n)) return {};

    return &StringSelectionNodeWidget::create;
}
