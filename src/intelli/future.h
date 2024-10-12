/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FUTURE_H
#define GT_INTELLI_FUTURE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <gt_platform.h>

#include <chrono>
#include <functional>

#include <QPointer>

namespace intelli
{

class GraphExecutionModel;

class ExecFuture
{
    friend class GraphExecutionModel;

    static constexpr size_t PRE_ALLOC = 5;

public:

    GT_INTELLI_EXPORT ~ExecFuture();

    using milliseconds = std::chrono::milliseconds;

    using CallbackFunctor = std::function<void(bool success)>;

    /**
     * @brief Waits for the evaluation of all target nodes. This is a blocking
     * call. However, the event loop will continue in the background.
     * An optional timeout may be specified, after which the future aborts the
     * waiting process. If a target node fails to evaluate, the waiting process
     * is aborted.
     * @param timeout Timeout to wait until the waiting process is aborted.
     * @return Success of evaluation/waiting process
     */
    GT_INTELLI_EXPORT
    bool wait(milliseconds timeout = milliseconds::max()) const;

    /**
     * @brief Waits for the evaluation of the specified node (and only for the
     * specified node, regardless of all other target nodes) and returns the
     * data at the specified port.
     * @param nodeUuid The target node to wait for its evaluation
     * @param portId The port to query the data from
     * @param timeout Timeout to wait until the waiting process is aborted.
     * @return Data at the specified port. Returns null if the process fails or
     * if the actual data is null
     */
    GT_NO_DISCARD
    GT_INTELLI_EXPORT
    NodeDataSet get(NodeUuid const& nodeUuid,
                    PortId portId,
                    milliseconds timeout = milliseconds::max()) const;

    GT_NO_DISCARD
    GT_INTELLI_EXPORT
    NodeDataSet get(NodeUuid const& nodeUuid,
                    PortType type,
                    PortIndex portIdx,
                    milliseconds timeout = milliseconds::max()) const;

    /**
     * @brief Registers a callback function that is called once the target
     * nodes have (failed) evaluation or if the timout has been reached.
     * @param functor Callback functor
     * @param timeout Timeout to wait until the callback function is called
     * with a failed status as parameter.
     * @return Reference for operator chaining.
     */
    GT_INTELLI_EXPORT
    ExecFuture const& then(CallbackFunctor functor,
                           milliseconds timeout = milliseconds::max()) const;

    /**
     * @brief Does not wait for the evaluation of all target nodes. Returns
     * whether the nodes started their evaluation successfully. Does not
     * invalidate the future, thus, it can be used to wait for the evaluation at
     * a later point in time.
     * @return Whether the evaluation prcocess started successfully
     */
    GT_INTELLI_EXPORT
    bool detach() const;

    GT_INTELLI_EXPORT
    bool startedSuccessfully() const { return detach(); }

    /**
     * @brief Joins with other futures. Can be used to wait for multiple,
     * separatly triggered nodes.
     * @param other Other future to join with
     * @return returns a new future
     */
    GT_INTELLI_EXPORT
    ExecFuture& join(ExecFuture const& other);

private:

    struct Impl;

    struct TargetNode
    {
        NodeUuid uuid;
        mutable NodeEvalState evalState;
    };

    /// pointer to the source execution model
    QPointer<GraphExecutionModel> m_model;
    /// targets to watch, contains only invalid or still evaluating targets
    QVector<TargetNode> m_targets;

    /**
     * @brief Constructs a future object from the model and registers the node
     * @param model Source exection model
     * @param nodeUuid Node to register
     * @param evalState Current node eval state
     */
    ExecFuture(GraphExecutionModel& model,
               NodeUuid nodeUuid,
               NodeEvalState evalState = NodeEvalState::Outdated);

    /**
     * @brief Constructs an empty future object from the model
     * @param model Source exection model
     */
    explicit ExecFuture(GraphExecutionModel& model);

    /**
     * @brief Registers the node to this future.
     * @param nodeUuid Node to register
     * @param evalState Current node eval state
     * @return this
     */
    ExecFuture& append(NodeUuid nodeUuid,
                       NodeEvalState evalState = NodeEvalState::Outdated);

    /**
     * @brief Returns whether all nodes have finished their evaluation
     * and are valid
     * @return Are nodes evaluated
     */
    bool areNodesEvaluated() const;

    /**
     * @brief Returns whether a node has failed evaluation.
     * @return Have nodes failed
     */
    bool haveNodesFailed() const;

    /**
     * @brief Updates evaluation state of all tracked targets.
     */
    void updateTargets() const;

    /**
     * @brief Resets the evaluation state of all tracked targets.
     */
    void resetTargets() const;
};

} // namespace intelli

#endif // GT_INTELLI_FUTURE_H
