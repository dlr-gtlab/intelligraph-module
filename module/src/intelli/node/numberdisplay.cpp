/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/numberdisplay.h"

#include "intelli/data/double.h"

#include <QLineEdit>

using namespace intelli;


NumberDisplayNode::NumberDisplayNode() :
    Node("Number Display")
{
    setNodeEvalMode(NodeEvalMode::MainThread);

    PortId in = addInPort(typeId<DoubleData>());

    registerWidgetFactory([=](){
        auto w = std::make_unique<QLineEdit>();
        w->setReadOnly(true);
        w->setFixedWidth(50);

        auto const updateText = [=, w_ = w.get()](){
            auto const& data = nodeData<DoubleData>(in);
            w_->setText(QString::number(data ? data->value() : 0));
        };
        
        connect(this, &Node::evaluated, w.get(), updateText);
        updateText();

        return w;
    });
}

