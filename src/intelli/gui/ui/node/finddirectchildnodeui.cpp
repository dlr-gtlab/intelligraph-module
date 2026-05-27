/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/finddirectchildnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/widgets/finddirectchildwidget.h>
#include <intelli/node/finddirectchild.h>

#include <QGraphicsWidget>

using namespace intelli;

FindDirectChildNodeWidget::FindDirectChildNodeWidget(FindDirectChildNode& node) :
    FindDirectChildWidget()
{
    m_node = &node;

    QObject::connect(this, &FindDirectChildWidget::updateClass,
                     m_node, &FindDirectChildNode::setTargetClassName);
    QObject::connect(this, &FindDirectChildWidget::updateObjectName,
                     m_node, &FindDirectChildNode::setTargetObjectName);

    QObject::connect(m_node, &FindDirectChildNode::targetClassNameChanged,
                     this, &FindDirectChildWidget::setClassNameWidget);
    QObject::connect(m_node, &FindDirectChildNode::targetObjectNameChanged,
                     this, &FindDirectChildWidget::setObjectNameWidget);
    QObject::connect(m_node, &Node::inputDataRecieved,
                     this, &FindDirectChildNodeWidget::updateNameCompleterFromNode);

    setClassNameWidget(m_node->targetClassName());
    setObjectNameWidget(m_node->targetObjectName());
    updateNameCompleterFromNode();
}

NodeUI::QGraphicsWidgetPtr
FindDirectChildNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<FindDirectChildNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<FindDirectChildNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
FindDirectChildNodeWidget::updateNameCompleterFromNode()
{
    if (!m_node) return;

    updateNameCompleter(m_node->inputObject());
}

FindDirectChildNodeUI::FindDirectChildNodeUI() = default;

NodeUI::WidgetFactoryFunction
FindDirectChildNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<FindDirectChildNode const*>(&n)) return {};

    return &FindDirectChildNodeWidget::create;
}
