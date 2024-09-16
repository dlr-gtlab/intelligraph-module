/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodeexecutor.h"
#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/graphexecmodel.h"

#include "intelli/exec/detachedexecutor.h"

#include "intelli/private/node_impl.h"

#include <gt_utilities.h>

using namespace intelli;

bool
intelli::blockingEvaluation(Node& node, GraphExecutionModel& model)
{
    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });

    emit node.computingStarted();

    NodeExecutor::evaluate(node);

    return true;
}

bool
intelli::detachedEvaluation(Node& node, GraphExecutionModel& model)
{
    if (node.findChild<DetachedExecutor*>())
    {
        gtError() << QObject::tr("Node %1 already has a executor!").arg(node.id());
        return false;
    }

    auto executor = new DetachedExecutor;
    executor->setParent(&node);

    return executor->evaluateNode(node, model);
}

void
NodeExecutor::evaluate(Node& node)
{
    node.eval();
}

GraphExecutionModel*
NodeExecutor::accessExecModel(Node& node)
{
    auto*  parent = qobject_cast<Graph*>(node.parent());
    return parent ? parent->executionModel() : nullptr;
}

void
NodeExecutor::setNodeDataInterface(Node& node, NodeDataInterface* interface)
{
    node.pimpl->dataInterface = interface;
}
