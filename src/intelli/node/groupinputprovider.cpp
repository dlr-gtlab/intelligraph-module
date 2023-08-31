/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/groupinputprovider.h"
#include "intelli/graphexecmodel.h"

#include "intelli/exec/sequentialexecutor.h"

using namespace intelli;

GroupInputProvider::GroupInputProvider() :
    AbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});
}

Node::NodeDataPtr
GroupInputProvider::eval(PortId outId)
{
    return nodeData(outId);
}

bool
GroupInputProvider::handleNodeEvaluation(GraphExecutionModel& model, PortIndex idx)
{
    SequentialExecutor executor;

    return executor.evaluateNode(*this, model, idx);
}
