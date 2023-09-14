/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/nodeexecutor.h"
#include "intelli/node.h"

#include "intelli/exec/detachedexecutor.h"
#include "intelli/exec/blockingexecutor.h"

using namespace intelli;

bool
intelli::blockingEvaluation(Node& node, GraphExecutionModel& model, PortId portId)
{
    BlockingExecutor executor;
    return executor.evaluateNode(node, model, portId);
}

bool
intelli::detachedEvaluation(Node& node, GraphExecutionModel& model, PortId portId)
{
    if (node.findChild<DetachedExecutor*>())
    {
        gtError() << QObject::tr("Node already has a executor!");
        return false;
    }

    auto executor = new DetachedExecutor;
    executor->setParent(&node);

    return executor->evaluateNode(node, model, portId);
}

NodeDataPtr
NodeExecutor::doEvaluate(Node& node, PortId portId)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output port '" << portId << "'";

    return node.eval(portId);
}

NodeDataPtr
NodeExecutor::doEvaluate(Node& node)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName() << "'";

    return node.eval(invalid<PortId>());
}

GraphExecutionModel*
NodeExecutor::accessExecModel(Node& node)
{
    return node.executionModel();
}
