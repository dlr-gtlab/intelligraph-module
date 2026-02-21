/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/intinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/input/intinput.h>
#include <intelli/gui/ui/node/numberinputnodeui_utils.h>
#include <intelli/gui/widgets/intinputwidget.h>

#include <QGraphicsWidget>

using namespace intelli;

IntInputNodeUI::IntInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
IntInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<IntInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<IntInputNode*>(&source);
        if (!node) return nullptr;

        return ui::detail::makeNumberInputWidget<IntInputNode, IntInputWidget, int>(node, object);
    };
}
