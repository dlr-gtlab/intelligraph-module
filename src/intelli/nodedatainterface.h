/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEDATAINTERFACE_H
#define GT_INTELLI_NODEDATAINTERFACE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <gt_finally.h>

namespace intelli
{

using NodeDataPtrList = std::vector<std::pair<PortId, NodeDataPtr>>;

/**
 * @brief The NodeDataInterface class.
 * Interface to access and set the data of a node port
 */
class NodeDataInterface
{
public:

    virtual ~NodeDataInterface() = default;

    virtual NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const = 0;
    virtual NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const = 0;

    virtual bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) = 0;
    virtual bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) = 0;

    virtual NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const = 0;

    /**
     * @brief Should be called to mark a node as failed.
     * @param nodeUuid Node that failed evaluation
     */
    virtual void setNodeEvaluationFailed(NodeUuid const& nodeUuid) {}

    /// Helper struct to scope the duration of a node evaluation
    struct NodeEvaluationEndedFunctor
    {
        inline void operator()() const noexcept
        {
            if (i) i->nodeEvaluationFinished(uuid);
        }
        NodeDataInterface* i{};
        NodeUuid uuid{};
    };

    using ScopedEvaluation = gt::Finally<NodeEvaluationEndedFunctor>;

    /**
     * @brief Scoped wrapper around `nodeEvaluationStarted` and
     * `nodeEvaluationFinished`
     * @param nodeUuid Node that is being evaluated
     * @return
     */
    ScopedEvaluation nodeEvaluation(NodeUuid const& nodeUuid)
    {
        nodeEvaluationStarted(nodeUuid);
        return gt::finally(NodeEvaluationEndedFunctor{this, nodeUuid});
    }

    /**
     * @brief Called once the given node has startet evaluation. This function
     * must be called only once and should always be followed by
     * `nodeEvaluationFinished`.
     * @param nodeUuid Node that startet evaluation.
     */
    virtual void nodeEvaluationStarted(NodeUuid const& nodeUuid) {}
    /**
     * @brief Called once the given node has finished evaluation. This function
     * must be called only once and should always followed a call to
     * `nodeEvaluationStarted`.
     * @param nodeUuid Node that startet evaluation.
     */
    virtual void nodeEvaluationFinished(NodeUuid const& nodeUuid) {}

};

} // namespace intelli

#endif // GT_INTELLI_NODEDATAINTERFACE_H
