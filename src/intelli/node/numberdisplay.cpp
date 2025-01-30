/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/numberdisplay.h"

#include "intelli/data/double.h"

#include <QLineEdit>

using namespace intelli;


NumberDisplayNode::NumberDisplayNode() :
    Node("Number Display")
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    PortId in = addInPort(makePort(typeId<DoubleData>()).setCaptionVisible(false));

    setNodeFlag(ResizableHOnly);

    registerWidgetFactory([=](){
        auto w = std::make_unique<QLineEdit>();
        w->setReadOnly(true);
        w->setMinimumWidth(75);
        w->resize(w->minimumSizeHint());

        auto const updateText = [=, w_ = w.get()](){
            auto const& data = nodeData<DoubleData>(in);
            w_->setText(QString::number(data ? data->value() : 0));
        };
        
        connect(this, &Node::evaluated, w.get(), updateText);
        updateText();

        return w;
    });
}

