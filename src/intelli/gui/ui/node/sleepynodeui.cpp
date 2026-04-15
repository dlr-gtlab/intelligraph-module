/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/sleepynodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/sleepy.h>

#include <gt_icons.h>

#include <QGraphicsWidget>
#include <QLabel>

using namespace intelli;

SleepyNodeUI::SleepyNodeUI() = default;

NodeUI::WidgetFactoryFunction
SleepyNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<SleepyNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<SleepyNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<QLabel>();

        auto reset = [w_ = w.get(), node](){
            if (node->hasInputData())
                w_->setPixmap(gt::gui::icon::check().pixmap(20, 20));
            else
                w_->setPixmap(gt::gui::icon::cross().pixmap(20, 20));
        };

        auto update = [w_ = w.get()](int progress){
            w_->setPixmap(progress != 100 ?
                gt::gui::icon::processRunningIcon(progress).pixmap(20, 20) :
                gt::gui::icon::check().pixmap(20, 20));
        };

        QObject::connect(node, &SleepyNode::timePassed, w.get(), update);
        QObject::connect(node, &Node::inputDataRecieved, w.get(), reset);

        reset();

        return convertToGraphicsWidget(std::move(w), object);
    };
}
