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

class GtObject;

namespace intelli
{

using NodeDataPtrList = std::vector<std::pair<PortId, NodeDataPtr>>;

class GraphUserVariables;
/**
 * @brief The NodeDataInterface class.
 * Interface to access and set the data of a node port
 */
class NodeDataInterface : public QObject
{
    Q_OBJECT

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

    /**
     * @brief Returns the user variables object if any exists.
     * @return User variables object (may be null)
     */
    virtual GraphUserVariables const* userVariables() const { return nullptr; }

    /**
     * @brief Returns the scope object used for evalauation. The scope object
     * is intended to to be used for interfacing with the datamodel.
     * By default the scope object s the current project, but it may be set to
     * a sub datatree or an external tree instead.
     * @return Scope object (may be null).
     */
    virtual GtObject* scope() { return nullptr; }

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
    GT_NO_DISCARD
    ScopedEvaluation nodeEvaluation(NodeUuid const& nodeUuid)
    {
        nodeEvaluationStarted(nodeUuid);
        return gt::finally(NodeEvaluationEndedFunctor{this, nodeUuid});
    }

    /**
     * @brief Called once the given node has started evaluation. This function
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
