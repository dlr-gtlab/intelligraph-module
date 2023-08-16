/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/sleepy.h"

#include "intelli/data/double.h"
#include "intelli/nodefactory.h"

#include "intelli/private/utils.h"

#include <QThread>

using namespace intelli;

static auto init_once = [](){
    return GT_INTELLI_REGISTER_NODE(SleepyNode, "Number")
}();

SleepyNode::SleepyNode() :
    Node("Sleeping Node"),
    m_timer("timer", tr("Timer"), tr("Timer"), GtUnit::Time, 5)
{
    registerProperty(m_timer);

    m_in  = addInPort(typeId<DoubleData>(), Required);
    m_out = addOutPort(typeId<DoubleData>());
}

Node::NodeDataPtr
SleepyNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto data = nodeData(m_in);

    gtDebug() << "# SLEEPING START" << m_timer.get() << "s" << data;

    QThread::sleep(m_timer.getVal("s"));

    gtDebug() << "# SLEEPING END";

    return data;
}
