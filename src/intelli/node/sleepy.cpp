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

#ifdef GTIG_DEVELOPER_PREVIEW
GTIG_REGISTER_NODE(SleepyNode, "Number");
#endif

SleepyNode::SleepyNode() :
    Node("Sleeping Node"),
    m_timer("timer", tr("Timer"), tr("Timer"), GtUnit::Time, 5)
{
    registerProperty(m_timer);

    m_in  = addInPort(typeId<GtIgDoubleData>(), Required);
    m_out = addOutPort(typeId<GtIgDoubleData>());
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



