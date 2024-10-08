/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
    m_timer("timer", tr("Timer"), tr("Timer"), 5)
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

    size_t updates = m_timer * 1000 / intervalMs;

    emit timePassed(0);

    for (size_t i = 0; i < updates; ++i)
    {
        GtEventLoop eventloop(intervalMs);
        eventloop.exec();
        gtTrace().verbose()
            << gt::quoted(caption(), "", ":")
            << "Sending update" << i << "of" << updates;
        emit timePassed((int)((i / (double)updates) * 100));
    }

    emit timePassed(100);

    setNodeData(m_out, std::move(data));
}
