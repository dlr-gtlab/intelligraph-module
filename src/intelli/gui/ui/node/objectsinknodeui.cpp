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

using namespace intelli;

ObjectSinkNodeUI::ObjectSinkNodeUI() = default;

NodeUI::WidgetFactoryFunction
ObjectSinkNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ObjectSink const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<ObjectSink*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<QPushButton>(QObject::tr("Export"));
        w->setEnabled(node->canExport());

        QObject::connect(node, &ObjectSink::exportEnabledChanged,
                         w.get(), [w_ = w.get()](bool enabled) {
            w_->setEnabled(enabled);
        });

        QObject::connect(w.get(), &QPushButton::clicked,
                         w.get(), [node]() { node->exportObject(); });

        return convertToGraphicsWidget(std::move(w), object);
    };
}
