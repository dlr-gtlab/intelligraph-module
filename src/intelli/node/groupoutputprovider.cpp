/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/node/groupoutputprovider.h>
#include <intelli/graphexecmodel.h>
#include <intelli/nodeexecutor.h>

using namespace intelli;

GroupOutputProvider::GroupOutputProvider() :
    AbstractGroupProvider("Output Provider")
{
    setPos({250, 0});
}

#if 0
bool
GroupOutputProvider::handleNodeEvaluation(GraphExecutionModel& model)
{
    Graph* graph = qobject_cast<Graph*>(parentObject());
    if (!graph) return false;

    emit computingStarted();
    auto finally = gt::finally([graph, this](){
        emit computingFinished();
        emit graph->computingFinished();
    });

    NodeId graphId = graph->id();

    GraphExecutionModel* parentModel = NodeExecutor::accessExecModel(*graph);
    if (!parentModel)
    {
        gtWarning() << tr("Failed to set output data of graph node %1! "
                          "(execution model of graph node not found)")
                           .arg(graphId);
        return false;
    }

    if (!parentModel->setNodeData(
            graphId, PortType::Out, model.nodeData(id(), PortType::In)))
    {
        gtWarning() << tr("Failed to set output data of graph node %1!")
                           .arg(graphId);
        return false;
    }
    return true;
}
#endif
