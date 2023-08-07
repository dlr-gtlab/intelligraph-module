/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igsleepynode.h"

#include "gt_igdoubledata.h"
#include "gt_intelligraphnodefactory.h"

#include "private/utils.h"

#include <QThread>

#ifdef GTIG_DEVELOPER_PREVIEW
GTIG_REGISTER_NODE(GtIgSleepyNode, "Number");
#endif

GtIgSleepyNode::GtIgSleepyNode() :
    GtIntelliGraphNode("Sleeping Node"),
    m_timer("timer", tr("Timer"), tr("Timer"), GtUnit::Time, 5)
{
    registerProperty(m_timer);

    m_in  = addInPort(gt::ig::typeId<GtIgDoubleData>(), Required);
    m_out = addOutPort(gt::ig::typeId<GtIgDoubleData>());
}

GtIntelliGraphNode::NodeData
GtIgSleepyNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto data = nodeData(m_in);

    gtDebug() << "# SLEEPING START" << m_timer.get() << "s" << data;

    QThread::sleep(m_timer.getVal("s"));

    gtDebug() << "# SLEEPING END";

    return data;
}



