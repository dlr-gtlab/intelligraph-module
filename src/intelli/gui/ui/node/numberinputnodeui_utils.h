/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_NUMBERINPUTNODEUI_UTILS_H
#define GT_INTELLI_NUMBERINPUTNODEUI_UTILS_H

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/nodeui.h>

#include <QWidget>

namespace intelli
{
namespace ui
{
namespace detail
{

template <typename NodeT, typename WidgetT, typename ValueT>
NodeUI::QGraphicsWidgetPtr
makeNumberInputWidget(NodeT* node, NodeGraphicsObject& object)
{
    using InputMode = typename WidgetT::InputMode;

    auto* w = new WidgetT(static_cast<InputMode>(node->inputModeValue()));

    auto updateRange = [node, w]() {
        w->setRange(node->value(), node->lowerBound(), node->upperBound());
    };

    auto updateMode = [node, w]() {
        w->setInputMode(static_cast<InputMode>(node->inputModeValue()));
        node->setUseBounds(w->useBounds());
    };

    QObject::connect(w, &WidgetT::valueComitted,
                     w, [node, w]() {
        ValueT newVal = w->value();
        if (node->value() != newVal) node->setValue(newVal);
    });
    QObject::connect(w, &WidgetT::minChanged,
                     w, [node, w]() {
        ValueT newVal = w->min();
        if (node->lowerBound() != newVal) node->setLowerBound(newVal);
    });
    QObject::connect(w, &WidgetT::maxChanged,
                     w, [node, w]() {
        ValueT newVal = w->max();
        if (node->upperBound() != newVal) node->setUpperBound(newVal);
    });

    QObject::connect(node, &NodeT::rangeChanged, w, updateRange);
    QObject::connect(node, &NodeT::inputModeChanged, w, updateMode);

    updateRange();
    updateMode();

    return NodeUI::convertToGraphicsWidget(std::unique_ptr<QWidget>(w), object);
}

} // namespace detail
} // namespace ui
} // namespace intelli

#endif // GT_INTELLI_NUMBERINPUTNODEUI_UTILS_H
