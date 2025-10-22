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
#include <intelli/node/booldisplay.h>
#include <intelli/node/input/boolinput.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/widgets/booldisplaywidget.h>

using namespace intelli;

BoolNodeUI::BoolNodeUI() = default;

NodeUI::WidgetFactoryFunction
BoolNodeUI::centralWidgetFactory(Node const& n) const
{
    if (qobject_cast<BoolDisplayNode const*>(&n))
    {
        return [](NodeGraphicsObject& object) {
            auto* node = qobject_cast<BoolDisplayNode*>(&object.node());

            using DisplayMode = BoolDisplayWidget::DisplayMode;

            bool success = node->m_displayMode.registerEnum<DisplayMode>();
            assert(success);

            auto mode = node->m_displayMode.getEnum<DisplayMode>();

            auto wPtr = std::make_unique<BoolDisplayWidget>(false, mode);
            auto* w = wPtr.get();
            w->setReadOnly(true);

            auto updateWidget = [node, w](){
                auto const& data = node->nodeData<BoolData>(node->m_in);
                w->setValue(data ? data->value() : false);
            };
            auto const updateMode= [node, w]() {
                w->setDisplayMode(node->m_displayMode.getEnum<DisplayMode>());
                emit node->nodeChanged();
            };

            QObject::connect(node, &Node::inputDataRecieved, w, updateWidget);
            QObject::connect(&node->m_displayMode, &GtAbstractProperty::changed, w, updateMode);

            updateWidget();
            updateMode();

            return wPtr;
        };
    }
    if (qobject_cast<BoolInputNode const*>(&n))
    {
        return [](NodeGraphicsObject& object) {
            auto* node = qobject_cast<BoolInputNode*>(&object.node());

            using DisplayMode = BoolDisplayWidget::DisplayMode;

            bool success = node->m_displayMode.registerEnum<DisplayMode>();
            assert(success);

            auto mode = node->m_displayMode.getEnum<DisplayMode>();

            auto wPtr = std::make_unique<BoolDisplayWidget>(false, mode);
            auto* w = wPtr.get();

            auto const updateProp = [node, w]() {
                if (w->value() != node->value()) node->setValue(w->value());
            };
            auto const updateWidget = [node, w]() {
                w->setValue(node->value());
            };
            auto const updateMode= [node, w]() {
                w->setDisplayMode(node->m_displayMode.getEnum<DisplayMode>());
                emit node->nodeChanged();
            };

            QObject::connect(w, &BoolDisplayWidget::valueChanged, node, updateProp);
            QObject::connect(&node->m_value, &GtAbstractProperty::changed, w, updateWidget);
            QObject::connect(&node->m_displayMode, &GtAbstractProperty::changed, w, updateMode);

            updateWidget();

            return wPtr;
        };
    }

    return nullptr;
}
