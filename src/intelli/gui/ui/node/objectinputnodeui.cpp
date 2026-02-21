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

using namespace intelli;

ObjectInputNodeUI::ObjectInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
ObjectInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ObjectInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<ObjectInputNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<GtPropertyObjectLinkEditor>();
        w->setObjectLinkProperty(&node->objectProperty());

        auto updateScope = [node, w_ = w.get()]() {
            auto* model = exec::nodeDataInterface(*node);
            w_->setScope(model ? model->scope() : node->objectProperty().object());
        };
        auto updateText = [w_ = w.get()]() {
            w_->updateText();
        };

        QObject::connect(node, &Node::evaluated, w.get(), updateScope);
        QObject::connect(node, &Node::evaluated, w.get(), updateText);

        updateScope();
        updateText();

        return convertToGraphicsWidget(std::move(w), object);
    };
}
