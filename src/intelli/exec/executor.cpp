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
#include "intelli/private/node_impl.h"

using namespace intelli;

Executor::Executor() = default;

bool
Executor::isReady() const
{
    return true;
}

Executor::NodeDataPtr
Executor::doEvaluate(Node& node, PortIndex idx)
{
    auto& p = *node.pimpl;

    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output idx '" << idx << "'";

    return node.eval(p.outPorts.at(idx).id());
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
