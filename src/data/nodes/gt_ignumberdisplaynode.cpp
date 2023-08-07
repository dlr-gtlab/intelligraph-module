/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_ignumberdisplaynode.h"

#include "gt_intelligraphnodefactory.h"
#include "gt_igdoubledata.h"

#include <QLineEdit>

GTIG_REGISTER_NODE(GtIgNubmerDisplayNode, "Number");

GtIgNubmerDisplayNode::GtIgNubmerDisplayNode() :
    GtIntelliGraphNode("Number Display")
{
    PortId in = addInPort(gt::ig::typeId<GtIgDoubleData>());

    registerWidgetFactory([=](){
        auto w = std::make_unique<QLineEdit>();
        w->setReadOnly(true);
        w->setFixedWidth(50);

        auto const updateText = [=, w_ = w.get()](){
            auto* data = nodeData<GtIgDoubleData*>(in);
            w_->setText(QString::number(data ? data->value() : 0));
        };

        connect(this, &GtIntelliGraphNode::inputDataRecieved, w.get(), updateText);
        updateText();

        return w;
    });
}

