/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/ui/node/boolnodeui.h>
#include <intelli/data/bool.h>

#include <intelli/gui/graphics/nodeobject.h>

using namespace intelli;

BoolDisplayNodeWidget::BoolDisplayNodeWidget(BoolDisplayNode& node) :
    BoolDisplayGraphicsWidget(false)
{
    m_node = &node;

    setReadOnly(true);

    QObject::connect(m_node, &Node::inputDataRecieved,
                     this, &BoolDisplayNodeWidget::updateValueFromNode);
    QObject::connect(&m_node->m_displayMode, &GtAbstractProperty::changed,
                     this, &BoolDisplayNodeWidget::updateDisplayModeFromNode);

    updateValueFromNode();
    updateDisplayModeFromNode();
}

NodeUI::QGraphicsWidgetPtr
BoolDisplayNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    Q_UNUSED(object);

    auto* node = qobject_cast<BoolDisplayNode*>(&source);
    if (!node) return nullptr;

    return std::make_unique<BoolDisplayNodeWidget>(*node);
}

void
BoolDisplayNodeWidget::updateValueFromNode()
{
    auto const& data = m_node->nodeData<BoolData>(m_node->m_in);
    setValue(data ? data->value() : false);
}

void
BoolDisplayNodeWidget::updateDisplayModeFromNode()
{
    using DisplayMode = BoolDisplayGraphicsWidget::DisplayMode;

    setDisplayMode(m_node->m_displayMode.getEnum<DisplayMode>());
    emit m_node->nodeChanged();
}

BoolInputNodeWidget::BoolInputNodeWidget(BoolInputNode& node) :
    BoolDisplayGraphicsWidget(false)
{
    m_node = &node;

    QObject::connect(this, &BoolDisplayGraphicsWidget::valueChanged,
                     this, &BoolInputNodeWidget::updateNodeValueFromWidget);
    QObject::connect(&m_node->m_value, &GtAbstractProperty::changed,
                     this, &BoolInputNodeWidget::updateWidgetValueFromNode);
    QObject::connect(&m_node->m_displayMode, &GtAbstractProperty::changed,
                     this, &BoolInputNodeWidget::updateDisplayModeFromNode);

    updateWidgetValueFromNode();
    updateDisplayModeFromNode();
}

NodeUI::QGraphicsWidgetPtr
BoolInputNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    Q_UNUSED(object);

    auto* node = qobject_cast<BoolInputNode*>(&source);
    if (!node) return nullptr;

    return std::make_unique<BoolInputNodeWidget>(*node);
}

void
BoolInputNodeWidget::updateNodeValueFromWidget(bool value)
{
    if (value != m_node->value())
    {
        m_node->setValue(value);
    }
}

void
BoolInputNodeWidget::updateWidgetValueFromNode()
{
    setValue(m_node->value());
}

void
BoolInputNodeWidget::updateDisplayModeFromNode()
{
    using DisplayMode = BoolDisplayGraphicsWidget::DisplayMode;

    setDisplayMode(m_node->m_displayMode.getEnum<DisplayMode>());
    emit m_node->nodeChanged();
}

BoolNodeUI::BoolNodeUI() = default;

NodeUI::WidgetFactoryFunction
BoolNodeUI::centralWidgetFactory(Node const& n) const
{
    if (qobject_cast<BoolDisplayNode const*>(&n)) return &BoolDisplayNodeWidget::create;
    if (qobject_cast<BoolInputNode const*>(&n)) return &BoolInputNodeWidget::create;

    return {};
}
