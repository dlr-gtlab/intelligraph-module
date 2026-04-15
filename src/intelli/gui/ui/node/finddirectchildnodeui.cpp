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

FindDirectChildNodeUI::FindDirectChildNodeUI() = default;

NodeUI::WidgetFactoryFunction
FindDirectChildNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<FindDirectChildNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<FindDirectChildNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<FindDirectChildWidget>();

        w->updateNameCompleter(node->inputObject());

        QObject::connect(w.get(), &FindDirectChildWidget::updateClass,
                         w.get(), [node](QString const& newClass) {
            node->setTargetClassName(newClass);
        });
        QObject::connect(node, &FindDirectChildNode::targetClassNameChanged,
                         w.get(), [w_ = w.get()](QString const&) {
            w_->updateClassText();
        });

        QObject::connect(w.get(), &FindDirectChildWidget::updateObjectName,
                         w.get(), [node](QString const& newObjName) {
            node->setTargetObjectName(newObjName);
        });
        QObject::connect(node, &FindDirectChildNode::targetObjectNameChanged,
                         w.get(), [w_ = w.get()](QString const&) {
            w_->updateNameText();
        });

        QObject::connect(node, &Node::inputDataRecieved,
                         w.get(), [node, w_ = w.get()](PortId) {
            w_->updateNameCompleter(node->inputObject());
        });

        w->setClassNameWidget(node->targetClassName());
        w->setObjectNameWidget(node->targetObjectName());

        return convertToGraphicsWidget(std::move(w), object);
    };
}
