/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/booldisplay.h>
#include <intelli/data/bool.h>

#include <intelli/gui/widgets/booldisplaywidget.h>

using namespace intelli;

BoolDisplayNode::BoolDisplayNode() :
    Node(QStringLiteral("Bool Display")),
    m_displayMode("displayMode",
                  tr("Display Mode"),
                  tr("Display Mode"))
{
    registerProperty(m_displayMode);

    setNodeEvalMode(NodeEvalMode::Blocking);

    m_in = addInPort(makePort(typeId<BoolData>())
                         .setCaptionVisible(false));

    registerWidgetFactory([this](){
        using DisplayMode = BoolDisplayWidget::DisplayMode;

        bool success = m_displayMode.registerEnum<DisplayMode>();
        assert(success);

        auto mode = m_displayMode.getEnum<DisplayMode>();

        auto wPtr = std::make_unique<BoolDisplayWidget>(0, mode);
        auto* w = wPtr.get();
        w->setReadOnly(true);

        auto updateWidget = [this, w](){
            auto const& data = nodeData<BoolData>(m_in);
            w->setValue(data ? data->value() : false);
        };
        auto const updateMode= [this, w]() {
            w->setDisplayMode(m_displayMode.getEnum<DisplayMode>());
            nodeChanged();
        };

        connect(this, &Node::inputDataRecieved, w, updateWidget);
        connect(&m_displayMode, &GtAbstractProperty::changed, w, updateMode);

        updateWidget();
        updateMode();

        return wPtr;
    });
}
