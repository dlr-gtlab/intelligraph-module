/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/sequentialexecutor.h"
#include "intelli/node.h"
#include "intelli/graphexecmodel.h"

#include "gt_utilities.h"

using namespace intelli;

SequentialExecutor::SequentialExecutor() = default;

template <typename Lambda>
bool evaluateHelper(Node& node,
                    GraphExecutionModel* model,
                    Lambda const& function)
{

    return function(*model, node);
}

bool
SequentialExecutor::evaluateNode(Node& node, GraphExecutionModel& model, PortId portId)
{
    auto const evaluatePort = [&node, &model](PortId port){
        auto data = doEvaluate(node, port);

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
        doEvaluate(node);

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
