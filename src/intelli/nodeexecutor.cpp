/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
intelli::blockingEvaluation(Node& node, GraphExecutionModel& model, PortId portId)
{
    auto const evaluatePort = [&node, &model](PortId port){
        auto data = NodeExecutor::doEvaluate(node, port);

        bool success = model.setNodeData(node.id(), port, std::move(data));

        emit node.evaluated(port);

        return success;
    };

    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });

    if (portId != invalid<PortId>())
    {
        if (portId >= node.ports(PortType::Out).size()) return false;

        emit node.computingStarted();

        return evaluatePort(portId);
    }

    emit node.computingStarted();

    auto const& outPorts = node.ports(PortType::Out);

    // trigger eval if no outport exists
    if (outPorts.empty())
    {
        NodeExecutor::doEvaluate(node);

        emit node.evaluated();

        return true;
    }

    bool success = true;

    // iterate over all output ports
    for (auto& port : outPorts)
    {
        success &= evaluatePort(port.id());
    }

    return success;
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
    auto*  parent = qobject_cast<Graph*>(node.parent());
    return parent ? parent->mainExecutionModel() : nullptr;
}

void
NodeExecutor::setNodeDataInterface(Node& node, NodeDataInterface* interface)
{
    node.pimpl->dataInterface = interface;
}
