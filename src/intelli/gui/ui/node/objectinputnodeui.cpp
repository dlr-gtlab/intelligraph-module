/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/objectinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/input/objectinput.h>
#include <intelli/nodedatainterface.h>

#include <gt_propertyobjectlinkeditor.h>

#include <QGraphicsWidget>
#include <QVBoxLayout>

using namespace intelli;

ObjectInputNodeWidget::ObjectInputNodeWidget(ObjectInputNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_editor = new GtPropertyObjectLinkEditor(this);
    lay->addWidget(m_editor);

    m_editor->setObjectLinkProperty(&m_node->objectProperty());

    QObject::connect(m_node, &Node::evaluated,
                     this, &ObjectInputNodeWidget::updateScope);
    QObject::connect(m_node, &Node::evaluated,
                     this, &ObjectInputNodeWidget::updateText);

    updateScope();
    updateText();
}

NodeUI::QGraphicsWidgetPtr
ObjectInputNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<ObjectInputNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<ObjectInputNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
ObjectInputNodeWidget::updateScope()
{
    auto* model = exec::nodeDataInterface(*m_node);
    m_editor->setScope(model ? model->scope() : m_node->objectProperty().object());
}

void
ObjectInputNodeWidget::updateText()
{
    m_editor->updateText();
}

ObjectInputNodeUI::ObjectInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
ObjectInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ObjectInputNode const*>(&n)) return {};

    return &ObjectInputNodeWidget::create;
}
