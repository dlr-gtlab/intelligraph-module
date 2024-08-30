/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/sleepy.h"

#include "intelli/data/double.h"

#include <gt_icons.h>
#include <gt_eventloop.h>

#include <QTimer>
#include <QLabel>
#include <QThread>

using namespace intelli;


SleepyNode::SleepyNode() :
    Node("Sleeping Node"),
    m_timer("timer", tr("Timer"), tr("Timer"), GtUnit::Time, 5)
{
    registerProperty(m_timer);

    m_in  = addInPort(typeId<DoubleData>(), Required);
    m_out = addOutPort(typeId<DoubleData>());

    registerWidgetFactory([this](){
        auto w = std::make_unique<QLabel>();

        auto reset = [w_ = w.get(), this](){
            if (nodeData<DoubleData>(m_in))
                w_->setPixmap(gt::gui::icon::check().pixmap(20, 20));
            else
                w_->setPixmap(gt::gui::icon::cross().pixmap(20, 20));
        };

        auto update = [w_ = w.get()](int progress){
            w_->setPixmap(progress != 100 ?
                gt::gui::icon::processRunningIcon(progress).pixmap(20, 20) :
                gt::gui::icon::check().pixmap(20, 20));
        };
        connect(this, &SleepyNode::timePassed, w.get(), update);
        connect(this, &Node::inputDataRecieved, w.get(), reset);

        reset();

        return w;
    });
}

void
SleepyNode::eval()
{
    auto data = nodeData(m_in);

    constexpr int intervalMs = 500;

    auto updates = m_timer * 1000 / intervalMs;

    emit timePassed(0);

    for (int i = 1; i < updates; ++i)
    {
        GtEventLoop eventloop(intervalMs);
        eventloop.exec();
        gtDebug().verbose() << "Sending update" << i << "of" << updates;
        emit timePassed((int)((i / (double)updates) * 100));
    }

    emit timePassed(100);

    setNodeData(m_out, std::move(data));
}
