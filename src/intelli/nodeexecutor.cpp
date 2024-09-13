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
#include "intelli/exec/detachedexecutor.h"
#include "intelli/private/node_impl.h"

#include <gt_utilities.h>


namespace intelli
{

/**
 * @brief The NodeExecutor class. Helper class to access private or protected
 * members of a Node used for the evaluation.
 */
class NodeExecutor /// TODO: rename
{
    NodeExecutor() = delete;

public:

    static void evaluateNode(Node& node)
    {
        node.eval();
    }

    static void setNodeDataInterface(Node& node, NodeDataInterface& interface)
    {
        node.pimpl->dataInterface = &interface;
    }

    static NodeDataInterface* nodeDataInterface(Node& node)
    {
        return node.pimpl->dataInterface;
    }

    static bool triggerNodeEvaluation(Node& node, NodeDataInterface& interface)
    {
        return node.handleNodeEvaluation(interface);
    }
};

} // namespace intelli

using namespace intelli;

bool
intelli::exec::blockingEvaluation(Node& node, NodeDataInterface& model)
{
    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });

    emit node.computingStarted();

    NodeExecutor::setNodeDataInterface(node, model);
    NodeExecutor::evaluateNode(node);

    return true;
}

bool
intelli::exec::detachedEvaluation(Node& node, NodeDataInterface& model)
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

bool
intelli::exec::triggerNodeEvaluation(Node& node, NodeDataInterface& model)
{
    return NodeExecutor::triggerNodeEvaluation(node, model);
}

void
intelli::exec::setNodeDataInterface(Node& node, NodeDataInterface& model)
{
    return NodeExecutor::setNodeDataInterface(node, model);
}

NodeDataInterface*
intelli::exec::nodeDataInterface(Node& node)
{
    return NodeExecutor::nodeDataInterface(node);
}
