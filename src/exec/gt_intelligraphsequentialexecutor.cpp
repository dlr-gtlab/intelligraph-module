/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphsequentialexecutor.h"
#include "gt_intelligraphnode.h"

using namespace gt::ig;

bool
GtIntelliGraphSequentialExecutor::canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx)
{
    if (m_evaluating)
    {
        gtWarning() << tr("Cannot evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }
    return GtIntellIGraphExecutor::canEvaluateNode(node, outIdx);
}

bool
GtIntelliGraphSequentialExecutor::evaluateNode(GtIntelliGraphNode& node)
{
    if (!canEvaluateNode(node)) return false;

    auto const& outPorts = node.ports(PortType::Out);
    auto const& inPorts  = node.ports(PortType::In);

    m_evaluating = true;
    auto finally = gt::finally([this](){ m_evaluating = false; });

    // trigger eval if no outport exists
    if (outPorts.empty() && !inPorts.empty())
    {
        doEvaluateAndDiscard(node);
        return true;
    }

    // iterate over all output ports
    for (PortIndex idx{0}; idx < outPorts.size(); ++idx)
    {
        bool success = doEvaluate(node, idx);

        success ? emit node.outDataUpdated(idx) :
                  emit node.outDataInvalidated(idx);
    }

    return true;
}

bool
GtIntelliGraphSequentialExecutor::evaluatePort(GtIntelliGraphNode& node, PortIndex idx)
{
    if (idx >= node.ports(PortType::Out).size()) return false;

    if (!canEvaluateNode(node, idx)) return false;

    m_evaluating = true;
    auto finally = gt::finally([this](){ m_evaluating = false; });

    bool success = doEvaluate(node, idx);

    success ? emit node.outDataUpdated(idx) :
              emit node.outDataInvalidated(idx);

    return true;
}
