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
SequentialExecutor::evaluateNode(Node& node, PortIndex idx)
{
    auto* model = accessExecModel(node);
    if (!model)
    {
        gtError() << QObject::tr("Graph execution model for node '%1' not found! "
                                 "Aborting evaluation").arg(node.objectName());
        return false;
    }

    auto const evaluatePort = [&node, model](PortIndex idx){
        auto data = doEvaluate(node, idx);

        model->setNodeData(node.id(), PortType::Out, idx, std::move(data));

        emit node.evaluated(idx);
    };

    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });

    if (idx != invalid<PortIndex>())
    {
        if (idx >= node.ports(PortType::Out).size()) return false;

        emit node.computingStarted();

        evaluatePort(idx);

        return true;
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

    // iterate over all output ports
    for (PortIndex idx{0}; idx < outPorts.size(); ++idx)
    {
        evaluatePort(idx);
    }

    return true;
}
