/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "boolinputnode.h"

#include <intelli/data/bool.h>
#include <intelli/gui/property_item/booldisplay.h>

#include <QLayout>
#include <QTimer>

using namespace intelli;
using DisplayMode = BoolDisplayWidget::DisplayMode;

BoolInputNode::BoolInputNode() :
    Node("Bool Input"),
    m_value("value", tr("Value"), tr("Current Value")),
    m_displayMode("displayMode",
                  tr("Display Mode"),
                  tr("Display Mode"))
{
    registerProperty(m_value);
    registerProperty(m_displayMode);

    setNodeEvalMode(NodeEvalMode::Blocking);

    m_out = addOutPort(makePort(typeId<BoolData>())
                           .setCaptionVisible(false));

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    registerWidgetFactory([this]() {
        bool success = m_displayMode.registerEnum<DisplayMode>();
        assert(success);

        auto mode = m_displayMode.getEnum<DisplayMode>();

        auto wPtr = std::make_unique<BoolDisplayWidget>(0, mode);
        auto* w = wPtr.get();

        auto const updateProp = [this, w]() {
            if (w->value() != value()) setValue(w->value());
        };
        auto const updateWidget = [this, w]() {
            w->setValue(value());
        };
        auto const updateMode= [this, w]() {
            w->setDisplayMode(m_displayMode.getEnum<DisplayMode>());
            emit nodeChanged();
        };

        connect(w, &BoolDisplayWidget::valueChanged, this, updateProp);
        connect(&m_value, &GtAbstractProperty::changed, w, updateWidget);
        connect(&m_displayMode, &GtAbstractProperty::changed, w, updateMode);

        updateWidget();

        return wPtr;
    });
}

bool
BoolInputNode::value() const
{
    return m_value.get();
}

void
BoolInputNode::setValue(bool value)
{
    m_value = value;
}

void
BoolInputNode::eval()
{
    setNodeData(m_out, std::make_shared<BoolData>(value()));
}
