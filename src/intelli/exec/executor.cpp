/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/executor.h"

#include "intelli/graphexecmodel.h"
#include "intelli/node.h"

using namespace intelli;

Executor::Executor() = default;

bool
Executor::isReady() const
{
    return true;
}

Executor::NodeDataPtr
Executor::doEvaluate(Node& node, PortId portId)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output port '" << portId << "'";

    return node.eval(portId);
}

Executor::NodeDataPtr
Executor::doEvaluate(Node& node)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName() << "'";

    return node.eval(invalid<PortId>());
}

GraphExecutionModel*
Executor::accessExecModel(Node& node)
{
    return node.executionModel();
}
