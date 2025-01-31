/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/logicdisplay.h"
#include "intelli/gui/widgets/logicdisplay.h"

#include <intelli/data/bool.h>

#include <QLayout>

using namespace intelli;

LogicDisplayNode::LogicDisplayNode() :
    Node(QStringLiteral("Logic Display"))
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    m_in = addInPort(typeId<BoolData>());

    registerWidgetFactory([this](){
        auto base = makeBaseWidget();

        auto* w = new LogicDisplayWidget();
        w->setReadOnly(true);
        base->layout()->addWidget(w);

        auto update = [this, w](){
            auto const& data = nodeData<BoolData>(m_in);
            w->setValue(data ? data->value() : false);
        };

        connect(this, &Node::inputDataRecieved, w, update);

        update();

        return base;
    });
}

