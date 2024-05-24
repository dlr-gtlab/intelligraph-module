/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/node/groupinputprovider.h>
#include <intelli/graphexecmodel.h>
#include <intelli/nodeexecutor.h>

#include <gt_eventloop.h>

#include <QtConcurrent>

using namespace intelli;

struct DependencyEvaluated
{
    GtEventLoop* eventLoop;
    QVector<NodeId> dependencies;

    void operator ()(NodeId nodeId)
    {
        if (dependencies.contains(nodeId))
        {
            dependencies.removeOne(nodeId);
        }
        if (dependencies.empty()) emit eventLoop->success();
    }
};

GroupInputProvider::GroupInputProvider() :
    AbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});

//    connect(this, &GroupInputProvider::dependenciesEvaluated,
//            this, &GroupInputProvider::onDdependenciesEvaluated,
    //            Qt::QueuedConnection);
}

void
GroupInputProvider::eval()
{
    for (auto const& port : ports(PortType::Out))
    {
        setNodeData(port.id(), nodeData(port.id()));
    }
}

#if 0
bool forwardData(GroupInputProvider* input,
                 GraphExecutionModel& model,
                 GraphExecutionModel* parentModel,
                 NodeId graphId)
{
    if (!model.setNodeData(
            input->id(), invert(input->providerType), parentModel->nodeData(graphId, input->providerType)))
    {
        gtWarning() << QObject::tr("Failed to forward input data of graph node %1!")
                           .arg(graphId);
        return false;
    }

    return true;
};

void
GroupInputProvider::onDdependenciesEvaluated()
{
    auto finally = gt::finally(this, &Graph::computingFinished);

    Graph* graph = qobject_cast<Graph*>(parentObject());
    if (!graph) return;
    Graph* superGraph = qobject_cast<Graph*>(graph->parentObject());
    if (!superGraph) return;
    GraphExecutionModel* parentModel = NodeExecutor::accessExecModel(*graph);
    if (!parentModel) return;
    GraphExecutionModel* model = NodeExecutor::accessExecModel(*this);
    if (!model) return;

    NodeId graphId = graph->id();
    auto const& dependencies = superGraph->findConnectedNodes(graphId, providerType);

    bool allEvaluated = std::all_of(dependencies.begin(), dependencies.end(),
                                    [parentModel](NodeId dependency){
        return parentModel->isNodeEvaluated(dependency);
    });
    if (!allEvaluated)
    {
        gtWarning() << tr("Not all input nodes were evaluated!");
        return;
    }

    gtWarning() << tr("Forwarding input data!");
    forwardData(this, *model, parentModel, graphId);
}
    Graph* graph = qobject_cast<Graph*>(parentObject());
    if (!graph) return false;

    emit computingStarted();
    auto finally = gt::finally(this, &Graph::computingFinished);

    NodeId graphId = graph->id();

    GraphExecutionModel* parentModel = NodeExecutor::accessExecModel(*graph);
    if (parentModel)
    {
        return forwardData(this, model, parentModel, graphId);
    }

    gtWarning() << tr("Failed to forward input data of graph node %1! "
                      "(execution model of graph node not found)")
                       .arg(graphId);

    Graph* superGraph = qobject_cast<Graph*>(graph->parentObject());
    if (!superGraph) return false;

    gtWarning() << tr("Attempting to create execution model...");

    parentModel = superGraph->makeDummyExecutionModel();
    assert(parentModel);

    auto const& dependencies = superGraph->findConnectedNodes(graphId, providerType);
    if (dependencies.empty()) return forwardData(this, model, parentModel, graphId);

    for (auto dependency : dependencies)
    {
        if (!parentModel->evaluateNode(dependency).detach()) return false;
    }

    finally.clear();

    QtConcurrent::run([dependencies,
                       execModel = QPointer<GraphExecutionModel>(parentModel),
                       inputProvider = QPointer<GroupInputProvider>(this)](){
        if (!inputProvider || !execModel) return;

        GtEventLoop eventLoop{std::chrono::minutes{1}};

        bool allEvaluated = std::all_of(dependencies.begin(), dependencies.end(),
                                        [execModel](NodeId dependency){
            return execModel->isNodeEvaluated(dependency);
        });

        if (allEvaluated)
        {
            return emit inputProvider->dependenciesEvaluated();
        }

        eventLoop.connectFailed(execModel.data(), &GraphExecutionModel::internalError);
        eventLoop.connectFailed(execModel.data(), &GraphExecutionModel::graphStalled);

        QObject::connect(execModel.data(), &GraphExecutionModel::nodeEvaluated,
                         &eventLoop, DependencyEvaluated{&eventLoop, std::move(dependencies)});

        gtWarning() << inputProvider << tr("waiting for input data...");
        eventLoop.exec();
        gtWarning() << inputProvider << tr("waiting for input data done!");
        if (inputProvider) emit inputProvider->dependenciesEvaluated();
    });

    return true;
}
#endif
