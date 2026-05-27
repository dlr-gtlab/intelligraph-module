/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/objectsinknodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/objectsink.h>

#include <QGraphicsWidget>
#include <QPushButton>
#include <QVBoxLayout>

using namespace intelli;

ObjectSinkNodeWidget::ObjectSinkNodeWidget(ObjectSink& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_button = new QPushButton(QObject::tr("Export"), this);
    lay->addWidget(m_button);

    m_button->setEnabled(m_node->canExport());

    QObject::connect(m_node, &ObjectSink::exportEnabledChanged,
                     this, &ObjectSinkNodeWidget::updateExportEnabled);
    QObject::connect(m_button, &QPushButton::clicked,
                     this, &ObjectSinkNodeWidget::exportObject);
}

NodeUI::QGraphicsWidgetPtr
ObjectSinkNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<ObjectSink*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<ObjectSinkNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
ObjectSinkNodeWidget::updateExportEnabled(bool enabled)
{
    m_button->setEnabled(enabled);
}

void
ObjectSinkNodeWidget::exportObject()
{
    m_node->exportObject();
}

ObjectSinkNodeUI::ObjectSinkNodeUI() = default;

NodeUI::WidgetFactoryFunction
ObjectSinkNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ObjectSink const*>(&n)) return {};

    return &ObjectSinkNodeWidget::create;
}
