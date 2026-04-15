/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/doubleinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/input/doubleinput.h>
#include <intelli/gui/ui/node/numberinputnodeui_utils.h>
#include <intelli/gui/widgets/doubleinputwidget.h>

#include <QGraphicsWidget>

using namespace intelli;

DoubleInputNodeUI::DoubleInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
DoubleInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<DoubleInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<DoubleInputNode*>(&source);
        if (!node) return nullptr;

        return ui::detail::makeNumberInputWidget<DoubleInputNode, DoubleInputWidget, double>(node, object);
    };
}
