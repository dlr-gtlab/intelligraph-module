/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 22.7.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#include <intelli/future.h>
#include <intelli/graphexecmodel.h>

#include <intelli/private/utils.h>

#include <gt_eventloop.h>

using namespace intelli;

struct FutureEvaluated::Impl
{
    static inline void
    setupEventLoop(FutureEvaluated const& future, GtEventLoop& loop)
    {
        loop.connectFailed(future.m_model.data(), &GraphExecutionModel::internalError);

        // react to node evaluation signals
        auto const& performUpdate = [&future, &loop](){
            future.updateTargets();
            if (future.areNodesEvaluated()) return emit loop.success();
            if (future.haveNodesFailed()) return emit loop.failed();
        };

        QObject::connect(future.m_model, &GraphExecutionModel::nodeEvaluated,
                         &loop, performUpdate);
        QObject::connect(future.m_model, &GraphExecutionModel::nodeEvaluationFailed,
                         &loop, performUpdate);

        performUpdate();
    }

    /// Helper class that invokes the functor member once all target nodes have
    /// evaluated. Exploits GtEventLoop structure.
    struct Observer : public GtEventLoop
    {
        FutureEvaluated future;
        CallbackFunctor functor;

        Observer(FutureEvaluated const& future_,
                 CallbackFunctor functor_) :
            GtEventLoop(0), // event loop wont be executed
            future(future_),
            functor(functor_)
        {
            setObjectName("__observer");
        }
    };
};

FutureEvaluated::FutureEvaluated(GraphExecutionModel& model) :
    m_model(&model)
{
    assert(m_model);
}

FutureEvaluated::FutureEvaluated(GraphExecutionModel& model,
                                 NodeUuid nodeUuid,
                                 NodeEvalState evalState) :
    FutureEvaluated(model)
{
    append(std::move(nodeUuid), evalState);
}

bool
FutureEvaluated::wait(milliseconds timeout) const
{
    GT_INTELLI_PROFILE();

    if (!m_model) return false;

    if (areNodesEvaluated()) return true; // nodes are already evaluated

    if (haveNodesFailed()) return false; // some nodes failed -> abort

    // Nodes are still running
    // -> Create local event loop here to start recieving updates from exec model
    GtEventLoop loop(timeout);
    Impl::setupEventLoop(*this, loop);

    // Nodes may have updated in the meantime (might not be necessary/possible?)
    if (areNodesEvaluated()) return true;
    if (haveNodesFailed()) return false;

    // Perform blocking wait
    GtEventLoop::State state = loop.exec();

    // Reset all targets, so that a subsequent `wait()` has to refetch all
    // their states
    resetTargets();

    return state == GtEventLoop::Success;
}

NodeDataSet
FutureEvaluated::get(NodeUuid const& nodeUuid,
                     PortId portId,
                     milliseconds timeout) const
{
    if (portId == invalid<PortId>()) return {};

    // create a local future to only wait for the one node
    FutureEvaluated future(const_cast<GraphExecutionModel&>(*m_model));
    future.append(nodeUuid, NodeEvalState::Outdated);

    if (!future.wait(timeout)) return {};

    // model may be deleted while waiting
    assert(m_model);
    return m_model->nodeData(nodeUuid, portId);
}

NodeDataSet
FutureEvaluated::get(NodeUuid const& nodeUuid,
                     PortType type,
                     PortIndex portIdx,
                     milliseconds timeout) const
{
    assert(m_model);
    auto* node = m_model->graph().findNodeByUuid(nodeUuid);
    if (!node) return {};

    return get(nodeUuid, node->portId(type, portIdx), timeout);
}

FutureEvaluated const&
FutureEvaluated::then(CallbackFunctor functor) const
{
    auto observer = std::make_unique<Impl::Observer>(*this, std::move(functor));

    auto const invokeFunctor = [o = observer.get()](bool success){
        // invoke functor once, then delete observer
        if (o->functor)
        {
            o->functor(success);
            o->functor = {};
        }
        delete o;
    };
    auto const onSuccess = [invokeFunctor](){
        invokeFunctor(true);
    };
    auto const onFailure = [invokeFunctor](){
        invokeFunctor(false);
    };

    QObject::connect(observer.get(), &Impl::Observer::success,
                     observer.get(), onSuccess);
    QObject::connect(observer.get(), &Impl::Observer::failed,
                     observer.get(), onFailure);
    QObject::connect(observer.get(), &Impl::Observer::abort,
                     observer.get(), onFailure);

    observer->setParent(m_model);

    Impl::setupEventLoop(observer->future, *observer);

    observer.release();

    return *this;
}

bool
FutureEvaluated::detach() const
{
    updateTargets();
    return areNodesEvaluated() || !haveNodesFailed();
}

FutureEvaluated&
FutureEvaluated::join(FutureEvaluated const& other)
{
    if (m_model != other.m_model)
    {
        gtError()
            << QObject::tr("Cannot to join futures, models are incompatible!");
        return *this;
    }

    for (auto const& targets : other.m_targets)
    {
        append(targets.uuid, targets.evalState);
    }
    return *this;
}

FutureEvaluated&
FutureEvaluated::append(NodeUuid nodeUuid, NodeEvalState evalState)
{
    switch (evalState)
    {
    case NodeEvalState::Outdated:
    case NodeEvalState::Evaluating:
        // node is evaluating or was evaluated
        break;
    case NodeEvalState::Paused:
    case NodeEvalState::Invalid:
        evalState = NodeEvalState::Invalid;
        break;
    case NodeEvalState::Valid:
        // only append nodes that are still running or invalid
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << "[FutureEvaluated]"
            << QObject::tr("Node %1 finished!").arg(nodeUuid);
#endif
        break;
    }

    auto iter = std::find_if(m_targets.begin(), m_targets.end(),
                             [nodeUuid](TargetNode const& target){
                                 return target.uuid == nodeUuid;
                             });

    // node is already a target, it is unclear which eval state to accept
    // -> mark as outdated
    if (iter != m_targets.end())
    {
        bool isInvalid = iter->evalState == NodeEvalState::Invalid ||
                         evalState != NodeEvalState::Invalid;

        iter->evalState = isInvalid ?
                              NodeEvalState::Invalid :
                              NodeEvalState::Outdated;
        return *this;
    }

    m_targets.push_back({std::move(nodeUuid), evalState});
    return *this;
}

bool
FutureEvaluated::areNodesEvaluated() const
{
    bool allValid = std::all_of(m_targets.begin(), m_targets.end(),
                                [](TargetNode const& target){
        return target.evalState == NodeEvalState::Valid;
    });
    return allValid;
}

bool
FutureEvaluated::haveNodesFailed() const
{
    bool anyInvalid = std::any_of(m_targets.begin(), m_targets.end(),
                                  [](TargetNode const& target){
        return target.evalState == NodeEvalState::Invalid;
    });
    return anyInvalid;
}

void
FutureEvaluated::updateTargets() const
{
    for (auto& target : m_targets)
    {
        NodeEvalState state = m_model->nodeEvalState(target.uuid);
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        if (target.evalState != state)
        {
            gtTrace().verbose()
                << "[FutureEvaluated]"
                << QObject::tr("Node %1 finished!").arg(target.uuid);
        }
#endif
        target.evalState = state;
    }
}

void
FutureEvaluated::resetTargets() const
{
    for (auto& target : m_targets)
    {
        target.evalState = NodeEvalState::Outdated;
    }
}


