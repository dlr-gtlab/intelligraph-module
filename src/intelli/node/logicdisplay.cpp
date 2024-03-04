/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/logicdisplay.h"
#include "intelli/gui/property_item/logic.h"

#include <intelli/data/bool.h>

#include <QLayout>

using namespace intelli;

LogicDisplayNode::LogicDisplayNode() :
    intelli::Node(QStringLiteral("Logic Display"))
{
    setNodeEvalMode(intelli::NodeEvalMode::MainThread);

    m_in = addInPort(intelli::typeId<intelli::BoolData>());

    registerWidgetFactory([this](){
        auto base = intelli::makeWidget();

        auto* w = new LogicDisplayWidget();
        w->setReadOnly(true);
        base->layout()->addWidget(w);

        auto update = [this, w](){
            auto* data = nodeData<intelli::BoolData>(m_in);
            w->setValue(data ? data->value() : false);
        };

        connect(this, &intelli::Node::inputDataRecieved, w, update);

        update();

        return base;
    });
}

