/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */
 
#ifndef GT_INTELLI_GRAPHEXECMODEL_IMPL_H
#define GT_INTELLI_GRAPHEXECMODEL_IMPL_H

#include <intelli/graphexecmodel.h>
#include <intelli/graph.h>
#include <intelli/node.h>

#include <intelli/private/utils.h>

#include <gt_utilities.h>
#include <gt_algorithms.h>

#include <gt_logging.h>

#include <QMutex>
#include <QMutexLocker>

#ifdef GT_INTELLI_DEBUG_NODE_EXEC

namespace intelli
{

inline int& getIndentation(GraphExecutionModel const& model)
{
    static QHash<GraphExecutionModel const*, int> indentation;
    auto iter = indentation.find(&model);
    if (iter == indentation.end())
    {
        iter = indentation.insert(&model, 0);
    }
    return iter.value();
}

inline QString makeIndentation(int indent)
{
    if (indent == 0) return {};
    return QStringLiteral(" ") + QStringLiteral("+").repeated(indent);
}

} // namespace intelli

// helper macros for more legible output
#define INTELLI_LOG_IMPL(MODEL, INDENT) \
    (MODEL).graph().objectName() + QChar{':'} + makeIndentation(INDENT)
#define INTELLI_LOG_SCOPE(MODEL) \
           auto undo_indentation__ = gt::finally([&](){ getIndentation((MODEL))--; }); \
    gtTrace().verbose() << INTELLI_LOG_IMPL(MODEL, getIndentation((MODEL))++)
#define INTELLI_LOG(MODEL) \
    gtTrace().verbose() << INTELLI_LOG_IMPL(MODEL, getIndentation((MODEL)))

#define INTELLI_LOG_WARN(MODEL) \
    gtWarning().verbose() << INTELLI_LOG_IMPL(MODEL, getIndentation((MODEL)))

#else
#define INTELLI_LOG_SCOPE(MODEL) if (false) gtTrace()
#define INTELLI_LOG(MODEL) if (false) gtTrace()
#define INTELLI_LOG_WARN(MODEL) INTELLI_LOG(MODEL)
#endif

namespace intelli
{

using namespace data_model;

using MakeErrorFunction = QString(*)(Graph const&);

inline QString setNodeDataError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("failed to set node data") + ',';
}

inline QString getNodeDataError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("failed to access node data") + ',';
}

inline QString evaluteNodeError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("failed to evaluate node") + ',';
}

inline QString autoEvaluteNodeError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("failed to auto evaluate node") + ',';
}

/// Helper struct to "hide" implementation details and template functions
struct GraphExecutionModel::Impl
{
    struct Synchronization
    {
        struct SynchronizationEntry
        {
            /// pointer to exec model
            QPointer<GraphExecutionModel> ptr = {};
            /// number of running nodes
            size_t runningNodes = {};
            /// Indicates whether an exclusive node is running
            bool isExclusiveNodeRunning = false;
        };

        /// mutex for coarse locking
        QMutex mutex;
        /// registered entries
        QVector<SynchronizationEntry> entries;

        /**
         * @brief Returns whether any graph exec model is currently evaluating
         * an exclusive node
         * @return Whether any exclusive node is being evaluated
         */
        bool isExclusiveNodeRunning()
        {
            return std::any_of(entries.begin(),
                               entries.end(),
                               [](SynchronizationEntry const& entry){
                return entry.isExclusiveNodeRunning;
            });
        }

        /**
         * @brief Returns whether any graph model is currently evaluating nodes
         * @return Whether any nodes are being evaluated
         */
        bool areNodesRunning()
        {
            return std::any_of(entries.begin(),
                               entries.end(),
                               [](SynchronizationEntry const& entry){
               return entry.runningNodes > 0;
           });
        }

        /**
         * @brief Returns the index of the given exec model
         * @param model Graph exec model
         * @return Index of exec model (should not be invalid)
         */
        int indexOf(GraphExecutionModel& model)
        {
            int n = entries.size();
            for (int idx = 0; idx < n; idx++)
            {
                auto& entry = entries.at(idx);
                if (entry.ptr == &model) return idx;
            }
            return -1;
        }

        /**
         * @brief Notifies all other models that a graph exec model has finished
         * evaluating all of its nodes such that other graph exec models may
         * start evalauting nodes themselfes.
         * @param model Exec model that has finished evalauting all nodes
         */
        void notify(GraphExecutionModel& model)
        {
            for (auto& entry : entries)
            {
                if (entry.ptr == &model) continue;
                assert(entry.ptr);
                emit entry.ptr->wakeup(QPrivateSignal());
            }
        }

        void update(GraphExecutionModel& model)
        {
            if (model.m_evaluatingNodes.size() > 0) return;

            QMutexLocker locker{&s_sync.mutex};
            auto idx = s_sync.indexOf(model);
            assert(idx >= 0);

            auto& entry = s_sync.entries[idx];
            entry.runningNodes = 0;
            entry.isExclusiveNodeRunning = false;

            locker.unlock();
            notify(model);
        }
    };

    /// Synchronization entity for evaluating nodes exclusively between multiple
    /// graphs
    static Synchronization s_sync;

    /// Wrapper to retrieve iterator type for `QHash` depending on `IsConst`
    template<bool IsConst>
    struct DataItemHelper;

    template<bool IsConst>
    struct PortDataItemHelper;

    using MutableDataItemHelper = DataItemHelper<false>;
    using ConstDataItemHelper = DataItemHelper<true>;

    using MutablePortDataItemHelper = PortDataItemHelper<false>;
    using ConstPortDataItemHelper = PortDataItemHelper<true>;

    /// Finds all end/leaf nodes of the graph.
    template<typename List>
    static inline void
    findLeafNodes(Graph const& graph, List& targetNodes)
    {
        auto& conModel = graph.connectionModel();
        for (auto& entry : conModel)
        {
            if (entry.ports(PortType::Out).empty() &&
                !targetNodes.contains(entry.node->uuid()))
            {
                targetNodes.push_back(entry.node->uuid());
            }
        }
    }

    static inline bool
    containsGraph(GraphExecutionModel const& model,
                     Graph const& graph)
    {
        // TODO: gt::find_lowest_ancestor does not work here
        auto* target = &model.graph();
        auto* g = &graph;
        while (g && g != target)
        {
            g = g->parentGraph();
        }
        return g == target;
    }

    template<typename ExecModel, typename NodeIdent>
    static inline DataItemHelper<is_const<ExecModel>::value>
    findData(ExecModel& model,
             apply_constness_t<ExecModel, Graph>& graph,
             apply_constness_t<ExecModel, Node>* node,
             NodeIdent const& nodeIdent,
             MakeErrorFunction makeError = {})
    {
        if (!node)
        {
            if (makeError) gtError()
                    << makeError(graph)
                    << QObject::tr("node %1:%2 not found!")
                           .arg(relativeNodePath(graph))
                           .arg(nodeIdent);
            return {};
        }

        auto iter = model.m_data.find(node->uuid());
        if (iter == model.m_data.end())
        {
            if (makeError) gtError()
                    << makeError(graph)
                    << QObject::tr("entry for node '%1' (%2) not found!")
                           .arg(relativeNodePath(*node))
                           .arg(node->id());
            return {};
        }

        return { &model, iter, node };
    }

    template<typename ExecModel>
    static inline DataItemHelper<is_const<ExecModel>::value>
    findData(ExecModel& model,
             NodeUuid const& nodeUuid,
             MakeErrorFunction makeError = {})
    {
        auto& graph = model.graph();
        auto* node = graph.findNodeByUuid(nodeUuid);
        return findData(model, graph, node, nodeUuid, makeError);
    }

    template<typename ExecModel>
    static inline DataItemHelper<is_const<ExecModel>::value>
    findData(ExecModel& model,
             apply_constness_t<ExecModel, Graph>& graph,
             NodeId nodeId,
             MakeErrorFunction makeError = {})
    {
        auto* node = graph.findNode(nodeId);
        return findData(model, graph, node, nodeId, makeError);
    }

    template<typename ExecModel>
    static inline DataItemHelper<is_const<ExecModel>::value>
    findData(ExecModel& model,
             NodeId nodeId,
             MakeErrorFunction makeError = {})
    {
        auto* graph = qobject_cast<Graph*>(model.sender());
        if (!graph)
        {
            if (makeError) gtError()
                    << makeError(model.graph())
                    << tr("graph node not found!");
            return {};
        }

        return findData(model, *graph, nodeId, makeError);
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 DataItemHelper<is_const<ExecModel>::value>& item,
                 PortId portId,
                 MakeErrorFunction makeError = {})
    {
        auto* portEntry = item.entry->findPort(portId);
        if (!portEntry)
        {
            if (makeError) gtError()
                    << makeError(model.graph())
                    << tr("port entry for node '%1' (%2), port %3 not found!")
                           .arg(relativeNodePath(*item.node))
                           .arg(item.node->id())
                           .arg(portId);
            return {};
        }
        PortType type = item.node->portType(portId);
        assert(type != PortType::NoType);

        return { item, portEntry, type };
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 DataItemHelper<is_const<ExecModel>::value>& item,
                 PortType type,
                 PortIndex portIdx,
                 MakeErrorFunction makeError = {})
    {
        PortId portId = item.node->portId(type, portIdx);
        if (portId == invalid<PortId>())
        {
            if (makeError) gtError()
                    << makeError(model.graph())
                    << tr("port entry for node '%1' (%2), port %3 %4 not found!")
                           .arg(relativeNodePath(*item.node))
                           .arg(item.node->id())
                           .arg(toString(type))
                           .arg(portIdx);
            return {};
        }
        return findPortData(model, item, portId, makeError);
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 NodeUuid const& nodeUuid,
                 PortType type,
                 PortIndex portIdx,
                 MakeErrorFunction makeError = {})
    {
        auto item = findData(model, nodeUuid, makeError);
        if (!item) return {};
        return findPortData(model, item, type, portIdx, makeError);
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 apply_constness_t<ExecModel, Graph>& graph,
                 NodeId nodeId,
                 PortType type,
                 PortIndex portIdx,
                 MakeErrorFunction makeError = {})
    {
        auto item = findData(model, graph, nodeId, makeError);
        if (!item) return {};
        return findPortData(model, item, type, portIdx, makeError);
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 NodeId nodeId,
                 PortType type,
                 PortIndex portIdx,
                 MakeErrorFunction makeError = {})
    {

        auto item = findData(model, nodeId, makeError);
        if (!item) return {};
        return findPortData(model, item, type, portIdx, makeError);
    }

    template<typename ExecModel>
    static inline PortDataItemHelper<is_const<ExecModel>::value>
    findPortData(ExecModel& model,
                 NodeUuid const& nodeUuid,
                 PortId portId,
                 MakeErrorFunction makeError = {})
    {
        auto item = findData(model, nodeUuid, makeError);
        if (!item) return {};
        return findPortData(model, item, portId, makeError);
    }

    template<bool IsConst>
    struct DataItemHelper
    {
        const_t<IsConst, GraphExecutionModel>* execModel = {};
        get_iterator_t<IsConst, GraphDataModel> entry = {};
        const_t<IsConst, Node>* node = {};

        operator bool() const { return execModel; }

        bool requiresReevaluation() const
        {
            return entry->state == NodeEvalState::Outdated;
        }

        bool inputsValid() const
        {
            auto& conModel = execModel->graph().globalConnectionModel();
            auto conData = conModel.find(node->uuid());
            if (conData == conModel.end()) return false;

            bool valid =
                std::all_of(entry->portsIn.begin(), entry->portsIn.end(),
                            [conData, this](PortDataItem const& entry){
                // TODO: check only predecessors
                bool isConnected = !conData->iterate(entry.portId).empty();
                bool isPortDataValid = entry.data.state == PortDataState::Valid;

                auto* port = node->port(entry.portId);
                bool hasRequiredData = port && (port->optional || entry.data.ptr);

                return (!isConnected || isPortDataValid) && hasRequiredData;
            });

            return valid;
        }

        bool isEvaluating() const
        {
            return utils::contains(execModel->m_evaluatingNodes, node->uuid());
        }

        bool isExclusive() const
        {
            return (size_t)node->nodeEvalMode() & IsExclusiveMask;
        }

        bool isQueued() const
        {
            return utils::contains(execModel->m_queuedNodes, node->uuid());
        }

        bool isEvaluated() const
        {
            return !isEvaluating() &&
                   (entry->state == NodeEvalState::Valid ||
                    entry->state == NodeEvalState::Invalid);
        }

        bool isReadyForEvaluation() const
        {
            return !isEvaluating() && inputsValid();
        }

        get_iterator_t<IsConst, GraphDataModel> operator->() { return entry; }
        get_iterator_t<IsConst, GraphDataModel> operator->() const { return entry; }
    };

    template<bool IsConst>
    struct PortDataItemHelper : DataItemHelper<IsConst>
    {
        using base_class = DataItemHelper<IsConst>;
        using base_class::operator bool;

        const_t<IsConst, PortDataItem>* portEntry{};
        PortType portType{PortType::NoType};

        PortDataItemHelper() = default;
        PortDataItemHelper(base_class item,
                           decltype(portEntry) _portEntry,
                           PortType _type) :
            base_class{item},
            portEntry(_portEntry),
            portType(_type)
        {}

        const_t<IsConst, PortDataItem>* operator->() { return portEntry; }
        const_t<IsConst, PortDataItem const>* operator->() const { return portEntry; }
    };

    /**
     * @brief Invalidates a node port such that the node and its output data
     * at the given port are considered outdated. A reevaluation is needed.
     * Propagates the invalidation to all connected successor nodes.
     * @param model Model
     * @param nodeUuid Node to update
     * @param portId Port to invalidate
     * @return success
     */
    static inline bool
    invalidatePort(GraphExecutionModel& model,
                   NodeUuid const& nodeUuid,
                   PortId portId)
    {
        auto item = findPortData(model, nodeUuid, portId);
        if (!item) return false;

        INTELLI_LOG(model)
            << tr("invalidating node '%1' (%2), port %4...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id())
                   .arg(portId);

        item->data.state = PortDataState::Outdated;

        // TODO: better solution?
        if (item.node->nodeEvalMode() != NodeEvalMode::ForwardInputsToOutputs)
        {
            return invalidateNode(model, nodeUuid, item);
        }

        // node is forwarding data from input to respective output
        item.entry->state = NodeEvalState::Outdated;
        emit item.node->nodeEvalStateChanged();

        PortType type = item.node->portType(portId);
        assert(type != PortType::NoType);

        switch (type)
        {
        case PortType::In:
        {
            // invalidate oppositte output port
            PortIndex idx = item.node->portIndex(PortType::In, item->portId);
            if (idx < 0 || (size_t)item.entry->portsOut.size() <= idx)
            {
                break;
            }
            invalidatePort(model, nodeUuid, item.node->portId(PortType::Out, idx));
            break;
        }
        case PortType::Out:
        {
            // invalidate all outgoing connections of this port only
            auto& conModel = model.graph().globalConnectionModel();
            for (auto& con : conModel.iterate(nodeUuid, portId))
            {
                invalidatePort(model, con.node, con.port);
            }
            break;
        }
        case PortType::NoType:
            throw GTlabException(__FUNCTION__, "path is unreachable!");
        }
        return true;
    }

    static inline bool
    invalidateNode(GraphExecutionModel& model,
                   NodeUuid const& nodeUuid)
    {
        auto item = Impl::findData(model, nodeUuid);
        if (!item) return false;

        return Impl::invalidateNode(model, nodeUuid, item);
    }

    /**
     * @brief Invalidates a node such that the node and its output data are
     * considered outdated. A reevaluation is needed. Propagates the
     * invalidation to all successor nodes.
     * @param model Model
     * @param nodeUuid Node to update
     * @param item Item
     * @return success
     */
    static inline bool
    invalidateNode(GraphExecutionModel& model,
                   NodeUuid const& nodeUuid,
                   MutableDataItemHelper item)
    {
        if (item->state == NodeEvalState::Outdated &&
            std::all_of(item->portsOut.begin(),
                        item->portsOut.end(),
                        [](auto const& port){
                return port.data.state == PortDataState::Outdated;
            }))
        {
            // already invalidated -> nothing to do
            return true;
        }

        INTELLI_LOG_SCOPE(model)
            << tr("invalidating node '%1' (%2)...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        item->state = NodeEvalState::Outdated;

        auto finally = gt::finally([node = item.node](){
            emit node->nodeEvalStateChanged();
        });

        if (!item.isReadyForEvaluation())
        {
            utils::erase(model.m_queuedNodes, nodeUuid);
        }

        bool success = true;
        for (auto& port : item->portsOut)
        {
            port.data.state = PortDataState::Outdated;

            // find and invalidate connected nodes
            auto& conModel = model.graph().globalConnectionModel();
            for (auto& con : conModel.iterate(nodeUuid, PortType::Out))
            {
                success &= invalidatePort(model, con.node, con.port);
            }
        }

        return success;
    }

    /**
     * @brief Propagates that a node has failed evaluating to all of its
     * successor nodes
     * @param model Model
     * @param nodeUuid Node to update
     * @param item Item
     */
    static void
    propagateNodeEvaluationFailure(GraphExecutionModel& model,
                                   NodeUuid const& nodeUuid,
                                   MutableDataItemHelper& item)
    {
        assert(item);
        if (item->state == NodeEvalState::Invalid) return;

        item->state = NodeEvalState::Invalid;
        emit item.node->nodeEvalStateChanged();

        auto& conModel = model.graph().globalConnectionModel();
        for (auto& successor : conModel.iterate(nodeUuid, PortType::Out))
        {
            auto subitem = findData(model, successor.node);
            if (!subitem) continue;

            propagateNodeEvaluationFailure(model, successor.node, subitem);
        }
    }

    /**
     * @brief Propagates that a node has started/is finished evaluating to
     * its parent graph. This is used to indicate that a subgraph is evaluating
     * even though the graph node itself is not evaluating currently.
     * @param model Model
     * @param graph Graph to update
     * @tparam Op Operation (plus/minus)
     */
    template <template <typename> class Op>
    static inline void
    propagateNodeEvalautionStatus(GraphExecutionModel& model, Graph* graph)
    {
        assert(graph);
        if (graph == graph->rootGraph()) return;

        auto item = Impl::findData(model, *graph, graph, graph->uuid());
        assert(item);

        // update counter
        auto& refCounter = item->m_evaluatingChildNodes;
        refCounter = Op<size_t>{}(refCounter, 1);
        assert(refCounter < std::numeric_limits<size_t>::max());
        emit item.node->nodeEvalStateChanged();

        // next parent
        propagateNodeEvalautionStatus<Op>(model, graph->parentGraph());
    }

    /// Helper method that sets the node data for the given node and port
    static inline bool
    setNodeData(GraphExecutionModel& model,
                NodeUuid const& nodeUuid,
                PortId portId,
                NodeDataSet data,
                bool tryTriggerEvaluation = true)
    {
        auto item = Impl::findPortData(model, nodeUuid, portId, setNodeDataError);
        if (!item) return false;

        return setNodeData(model,
                           item,
                           std::move(data),
                           tryTriggerEvaluation);
    }

    /// Helper method that sets the node data for the given node and port
    static inline bool
    setNodeData(GraphExecutionModel& model,
                MutableDataItemHelper item,
                PortId portId,
                NodeDataSet data,
                bool tryTriggerEvaluation = true)
    {
        auto portItem = Impl::findPortData(model, item, portId, setNodeDataError);
        if (!portItem) return false;

        return setNodeData(model,
                           portItem,
                           std::move(data),
                           tryTriggerEvaluation);
    }

    /// Helper method that sets the node data for the given node and port
    static inline bool
    setNodeData(GraphExecutionModel& model,
                MutablePortDataItemHelper item,
                NodeDataSet data,
                bool tryTriggerEvaluation = true)
    {
        assert(item);

        NodeUuid const& nodeUuid = item.node->uuid();
        PortId portId = item->portId;

        INTELLI_LOG_SCOPE(model)
            << tr("setting node data '%1' for node '%2' at port '%3'...")
                   .arg(toString(data.ptr), relativeNodePath(*item.node))
                   .arg(portId);

        item->data = std::move(data);

        switch (item.portType)
        {
        case PortType::In:
        {
            invalidateNode(model, nodeUuid, item);

            emit item.node->inputDataRecieved(portId);

            if (!tryTriggerEvaluation || model.isBeingModified()) break;

            // this node is evaluating
            if (utils::contains(model.m_evaluatingNodes, nodeUuid)) break;

            // check if predecessor is evaluating
            auto& conModel = model.graph().globalConnectionModel();
            auto predeccessors = conModel.iterateUniqueNodes(nodeUuid, PortType::In);
            bool arePredecessorsEvaluating =
                std::any_of(predeccessors.begin(),
                            predeccessors.end(),
                            [&model](NodeUuid const& predecessor){
                bool x = utils::contains(model.m_evaluatingNodes, predecessor);
                return x;
            });

            if (arePredecessorsEvaluating) break;

            INTELLI_LOG_SCOPE(model)
                << tr("triggering successor nodes...");

            bool triggeredEvaluation = false;
            triggeredEvaluation |= Impl::rescheduleTargetNodes(model);

            if (isNodeAutoEvaluating(model, nodeUuid))
            {
                triggeredEvaluation |= scheduleForAutoEvaluation(model, nodeUuid);
            }

            // evaluate next in queue if a new node was scheduled
            if (triggeredEvaluation)
            {
                Impl::evaluateNextInQueue(model);
            }
            break;
        }
        case PortType::Out:
        {
            if (item.requiresReevaluation())
            {
                item->data.state = PortDataState::Outdated;
                emit item.node->nodeEvalStateChanged();
            }

            // iterate over all connected ports
            auto& conModel = model.graph().globalConnectionModel();
            for (auto& con : conModel.iterate(nodeUuid, portId))
            {
                setNodeData(model,
                                  con.node,
                                  con.port,
                                  item->data,
                                  tryTriggerEvaluation);
            }
            break;
        }
        case PortType::NoType:
            throw GTlabException(__FUNCTION__, "path is unreachable!");
        }

        return true;
    }

    /// Helper method that accumulates all dependencies recursively of the given
    /// node and appens them to the spcified list.
    template<typename List>
    static void
    accumulateDependencies(GlobalConnectionModel const& conModel,
                           List& list,
                           NodeUuid const& nodeUuid,
                           PortType type = PortType::In)
    {
        if (utils::contains(list, nodeUuid)) return;

        list.push_back(nodeUuid);
        for (auto& nextNode : conModel.iterateNodes(nodeUuid, type))
        {
            accumulateDependencies(conModel, list, nextNode, type);
        }
    }

    /// Sorts the given list according to topo sort, such that all nodes
    /// at the start of the list have no dependencies
    template <typename List>
    static void
    sortDependencies(GraphExecutionModel& model, List& list)
    {
        auto& conModel = model.graph().globalConnectionModel();

        std::map<NodeUuid, std::vector<NodeUuid>> adjacencyMatrix;
        for (auto& nodeUuid : list)
        {
            std::vector<NodeUuid> predecessors;
            for (auto& predecessor : conModel.iterateUniqueNodes(nodeUuid, PortType::Out))
            {
                predecessors.push_back(predecessor);
            }
            adjacencyMatrix.insert({nodeUuid, predecessors});
        }
        list = gt::topo_sort(adjacencyMatrix);
    }

    /**
     * @brief Reschedules all target nodes and appends them to the list of
     * pending nodes
     * @param model Exec model
     * @return Whether any node was scheduled
     */
    static inline bool
    rescheduleTargetNodes(GraphExecutionModel& model)
    {
        model.m_pendingNodes.clear();

        if (model.m_targetNodes.empty()) return false;

        auto& conModel = model.graph().globalConnectionModel();

        // reschedule target nodes
        for (NodeUuid const& nodeUuid : model.m_targetNodes)
        {
            accumulateDependencies(conModel, model.m_pendingNodes, nodeUuid);
        }

        sortDependencies(model, model.m_pendingNodes);

        INTELLI_LOG(model) << "pending nodes:"
                           << model.m_pendingNodes;

        return schedulePendingNodes(model);
    }

    /**
     * @brief Reschedules all graphs marked for auto evalaution and appends
     * the nodes to the list of nodes to auto evaluate
     * @param model Exec model
     * @return Whether any node was scheduled
     */
    static inline bool
    rescheduleAutoEvaluatingNodes(GraphExecutionModel& model)
    {
        model.m_autoEvaluatingNodes.clear();

        if (model.m_autoEvaluatingGraphs.empty()) return false;

        QVarLengthArray<NodeUuid, 20> targets;

        // find all target nodes
        for (NodeUuid const& graphUuid : model.m_autoEvaluatingGraphs)
        {
            Graph const* graph = qobject_cast<Graph const*>(model.graph().findNodeByUuid(graphUuid));
            assert(graph);
            findLeafNodes(*graph, targets);

            // append the graph node itself
            bool isRootGraph = model.m_graph == graph;
            if (!isRootGraph) targets.push_back(graphUuid);
        }

        if (targets.empty()) return false;

        auto& conModel = model.graph().globalConnectionModel();

        std::vector<NodeUuid> dummy;
        // reschedule target nodes
        for (NodeUuid const& nodeUuid : targets)
        {
            accumulateDependencies(conModel, dummy, nodeUuid);
        }

        model.m_autoEvaluatingNodes = {dummy.begin(), dummy.end()};

        return scheduleAutoEvaluatingNodes(model);
    }

    /**
     * @brief Returns whether the given node should be auto evaluated.
     * @param model Model
     * @param nodeUuid Node to check
     * @return Whether to auto evalauate the given node
     */
    static inline bool
    isNodeAutoEvaluating(GraphExecutionModel const& model,
                       NodeUuid const& nodeUuid)
    {
        return model.m_autoEvaluatingNodes.find(nodeUuid) != model.m_autoEvaluatingNodes.end();
    }

    /**
     * @brief Auto evaluates the specified graph such that all dependencies
     * required for the evaluation of the leaf nodes of the specified graph are
     * marked for auto evaluation and are kept up-to-date.
     * @param model Exec model
     * @return success
     */
    static inline bool
    autoEvaluateGraph(GraphExecutionModel& model,
                      Graph& graph)
    {
        if (!model.isAutoEvaluatingGraph(graph))
        {
            model.m_autoEvaluatingGraphs.push_back(graph.uuid());
        }

        INTELLI_LOG_SCOPE(model)
            << QObject::tr("auto evaluating graph '%1'...")
                   .arg(relativeNodePath(graph));

        rescheduleAutoEvaluatingNodes(model);

        evaluateNextInQueue(model);

        emit model.autoEvaluationChanged(&graph);

        return true;
    }

    /**
     * @brief Schedules all successor nodes of the given node for auto
     * evaluation as long as they are marked for auto evaluation.
     * @param model Exec model
     * @return success
     */
    static inline bool
    scheduleAutoEvaluationOfSuccessors(GraphExecutionModel& model,
                                       NodeUuid const& nodeUuid)
    {
        auto& conModel = model.graph().globalConnectionModel();
        auto successors = conModel.iterateUniqueNodes(nodeUuid, PortType::Out);
        if (successors.empty()) return true;

        INTELLI_LOG_SCOPE(model)
            << tr("scheduling successor nodes for auto evaluation...");

        bool success = false;
        for (NodeUuid const& successor : successors)
        {
            if (isNodeAutoEvaluating(model, successor))
            {
                success |= scheduleForAutoEvaluation(model, successor);
            }
        }
        return success;
    }

    /**
     * @brief Schedules the given node for auto evaluation.
     * @param model Exec model
     * @return success
     */
    static inline bool
    scheduleForAutoEvaluation(GraphExecutionModel& model,
                              NodeUuid const& nodeUuid)
    {
        auto item = findData(model, nodeUuid, autoEvaluteNodeError);
        if (!item)
        {
            model.m_autoEvaluatingNodes.clear();
            return false;
        }

        INTELLI_LOG_SCOPE(model)
            << tr("attempting to queue node '%1' for auto evaluation...")
                   .arg(relativeNodePath(*item.node));

        if (!item.node->isActive())
        {
            INTELLI_LOG(model)
                << tr("node is paused!");
            return false;
        }

        if (item.isEvaluated())
        {
            INTELLI_LOG(model)
                << tr("node is already evaluated!");
            return scheduleAutoEvaluationOfSuccessors(model, nodeUuid);
        }

        if (item.isEvaluating())
        {
            INTELLI_LOG(model)
                << tr("node is already evaluating!");
            return true;
        }

        if (!item.isReadyForEvaluation())
        {
            INTELLI_LOG(model)
                << tr("node is not ready for evaluation!");
            return false;
        }

        if (item.isQueued())
        {
            INTELLI_LOG(model)
                << tr("node is already queued!");
            return true;
        }

        model.m_queuedNodes.push_back(nodeUuid);
        return true;
    }

    /**
     * @brief Evaluates the specified graph such that all dependencies that are
     * required for the evaluation of the leaf nodes of the specified graph are
     * evalauted exactly once.
     * @param model Exec model
     * @return success
     */
    static inline ExecFuture
    evaluateGraph(GraphExecutionModel& model,
                  Graph const& graph)
    {
        assert(containsGraph(model, graph));

        INTELLI_LOG_SCOPE(model)
            << QObject::tr("evaluating graph '%1'...")
                   .arg(relativeNodePath(graph));

        QVarLengthArray<NodeUuid, 10> targets;
        findLeafNodes(graph, targets);

        // append the graph node itself
        bool isRootGraph = model.m_graph == &graph;
        if (!isRootGraph) targets.push_back(graph.uuid());

        // evaluate pending nodes
        ExecFuture future{model};

        for (NodeUuid const& nodeUuid : targets)
        {
            INTELLI_LOG(model)
                << QObject::tr("scheduling target node '%1'...")
                       .arg(nodeUuid);

            if (!utils::contains(model.m_targetNodes, nodeUuid))
            {
                model.m_targetNodes.push_back(nodeUuid);
            }

            future.append(nodeUuid);
        }

        rescheduleTargetNodes(model);

        evaluateNextInQueue(model);

        return future;
    }

    /**
     * @brief Evaluates the specified node such that all dependencies that are
     * required for the evaluation are evalauted exactly once.
     * @param model Exec model
     * @return success
     */
    static inline ExecFuture
    evaluateNode(GraphExecutionModel& model,
                 NodeUuid const& nodeUuid)
    {
        INTELLI_LOG_SCOPE(model)
            << QObject::tr("scheduling target node '%1'...")
                   .arg(nodeUuid);

        // append to target nodes
        if (!utils::contains(model.m_targetNodes, nodeUuid))
        {
            model.m_targetNodes.push_back(nodeUuid);
        }

        // reschedule pending nodes
        rescheduleTargetNodes(model);

        evaluateNextInQueue(model);

        return ExecFuture{model, nodeUuid};
    }

    /**
     * @brief Schedules all pending nodes for evaluation that are also ready for
     * evaluation but does not trigger their evaluation. Assumes a topological
     * sorted list of pending nodes.
     * @param model Exec model
     * @return success
     */
    static inline bool
    schedulePendingNodes(GraphExecutionModel& model)
    {
        if (model.m_pendingNodes.empty()) return false;

        INTELLI_LOG_SCOPE(model)
            << tr("scheduling pending nodes...");

        size_t before = model.m_pendingNodes.size();

        for (size_t idx = 0; idx < model.m_pendingNodes.size(); ++idx)
        {
            NodeUuid const& nodeUuid = model.m_pendingNodes.at(idx);

            auto item = findData(model, nodeUuid, evaluteNodeError);
            if (!item)
            {
                model.m_pendingNodes.clear();
                return false;
            }

            auto removeFromPending = gt::finally([&model, &idx](){
                auto iter = model.m_pendingNodes.begin() + idx--;
                model.m_pendingNodes.erase(iter);
            });
            Q_UNUSED(removeFromPending);

            INTELLI_LOG_SCOPE(model)
                << tr("attempting to queue node '%1'...")
                       .arg(relativeNodePath(*item.node));

            if (item.isEvaluated())
            {
                INTELLI_LOG(model)
                    << tr("node is already evaluated!");
                continue;
            }

            if (item.isEvaluating())
            {
                INTELLI_LOG(model)
                    << tr("node is already evaluating!");
                continue;
            }

            if (!item.isReadyForEvaluation())
            {
                INTELLI_LOG(model)
                    << tr("node is not ready for evaluation!");

                removeFromPending.clear();
                break;
            }

            if (item.isQueued())
            {
                INTELLI_LOG(model)
                    << tr("node is already queued!");
                continue;
            }

            model.m_queuedNodes.push_back(nodeUuid);
        }

        size_t after = model.m_pendingNodes.size();

        return (after - before) > 0;
    }

    /**
     * @brief Schedules all nodes that are marked for auto evaluation but
     * does not trigger their evaluation.
     * @param model Exec model
     * @return success
     */
    static inline bool
    scheduleAutoEvaluatingNodes(GraphExecutionModel& model)
    {
        if (model.m_autoEvaluatingNodes.empty()) return false;

        // dummy vector to avoid iterating through entire set of nodes
        std::vector<NodeUuid> dummy{
            model.m_autoEvaluatingNodes.begin(),
            model.m_autoEvaluatingNodes.end()
        };
        sortDependencies(model, dummy);

        INTELLI_LOG_SCOPE(model)
            << tr("scheduling auto evaluating nodes:")
            << dummy;

        bool success = !dummy.empty();
        for (NodeUuid const& nodeUuid : dummy)
        {
            if (!scheduleForAutoEvaluation(model, nodeUuid))
            {
                return false;
            }
        }
        return success;
    }

    /**
     * @brief Tries to evalaute the specified node.
     * @param model Exec model
     * @return success
     */
    template <typename Iter>
    static inline NodeEvalState
    tryEvaluatingNode(GraphExecutionModel& model,
                      MutableDataItemHelper item,
                      Iter iter)
    {
        assert(item);
        if (!item.isReadyForEvaluation())
        {
            return NodeEvalState::Outdated;
        }

        if (model.isBeingModified())
        {
            INTELLI_LOG(model)
                << tr("executor is being modified!");
            return NodeEvalState::Paused;
        }

        // check if this model has
        bool isExclusiveNodeRunning =
            std::any_of(model.m_evaluatingNodes.cbegin(),
                        model.m_evaluatingNodes.cend(),
                        [&model](NodeUuid const& nodeUuid){
            auto item = findData(model, nodeUuid);
            assert(item);
            return item.isExclusive();
        });

        // an exclusive node has to be evaluated separatly to all other nodes
        if (isExclusiveNodeRunning)
        {
            INTELLI_LOG(model)
                << tr("executor is evaluating an exclusive node!");
            return NodeEvalState::Paused;
        }

        bool isExclusive = item.isExclusive();
        if (isExclusive && !model.m_evaluatingNodes.empty())
        {
            INTELLI_LOG(model)
                << tr("node is exclusive and must wait for others to finish!");
            return NodeEvalState::Paused;
        }

        // check other models
        {
            QMutexLocker locker{&s_sync.mutex};

            // an exclusive node has to be evaluated separatly to all other nodes
            if (s_sync.isExclusiveNodeRunning())
            {
                INTELLI_LOG(model)
                    << tr("an other executor is evaluating an exclusive node!");
                return NodeEvalState::Paused;
            }

            // exclusive node cannot be evaluated yet
            if (isExclusive && s_sync.areNodesRunning())
            {
                INTELLI_LOG(model)
                    << tr("node is exclusive and must wait for other models to finish!");
                return NodeEvalState::Paused;
            }

            // update synchronization entitiy
            auto idx = s_sync.indexOf(model);
            assert(idx >= 0);
            auto& entry = s_sync.entries[idx];
            entry.runningNodes += 1;
            entry.isExclusiveNodeRunning = isExclusive;
        }

        INTELLI_LOG_SCOPE(model)
            << tr("triggering evaluation of node '%1'...")
                   .arg(relativeNodePath(*item.node));

        NodeUuid const& nodeUuid = item.node->uuid();

        // dequeue and mark as evaluating
        assert(model.m_queuedNodes.end() != iter);
        model.m_queuedNodes.erase(iter);

        // trigger node evaluation
        if (!exec::triggerNodeEvaluation(*item.node, model))
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("node execution failed!");

            // update synchronization entity
            {
                QMutexLocker locker{&s_sync.mutex};

                auto idx = s_sync.indexOf(model);
                assert(idx >= 0);

                auto& entry = s_sync.entries[idx];
                entry.runningNodes -= 1;
                if (isExclusive) entry.isExclusiveNodeRunning = false;

                locker.unlock();
                s_sync.notify(model);
            }

            propagateNodeEvaluationFailure(model, nodeUuid, item);

            return NodeEvalState::Invalid;
        }

        return item->state;
    }

    /**
     * @brief Attempts to evalaute all queued nodes.
     * @param model Exec model
     * @return success
     */
    static inline bool
    evaluateNextInQueue(GraphExecutionModel& model)
    {
        if (model.m_queuedNodes.empty()) return false;

        INTELLI_LOG_SCOPE(model)
            << "evaluating next in queue:"
            << std::vector<NodeUuid>{model.m_queuedNodes.begin(), model.m_queuedNodes.end()} << "...";

        // do not evaluate if graph is currently being modified
        if (model.isBeingModified())
        {
            INTELLI_LOG(model)
                << tr("model is being modified!");
            return false;
        }

        bool triggeredNodes = false;

        // for each node in queue
        for (auto iter = model.m_queuedNodes.begin();
             iter != model.m_queuedNodes.end();
             iter  = model.m_queuedNodes.begin())
        {
            auto const& nodeUuid = *iter;

            auto item = findData(model, nodeUuid);
            if (!item)
            {
                gtError() << evaluteNodeError(model.graph())
                          << tr("node %1 not found!")
                                 .arg(nodeUuid);

                model.m_queuedNodes.erase(iter);
                continue;
            }

            auto state = tryEvaluatingNode(model, item, iter);
            switch (state)
            {
            case NodeEvalState::Valid:
            case NodeEvalState::Evaluating:
                triggeredNodes = true;
                continue;
            case NodeEvalState::Invalid:
            case NodeEvalState::Outdated:
                continue;
            case NodeEvalState::Paused:
                return triggeredNodes;
            }
        }

        if (!triggeredNodes)
        {
            INTELLI_LOG(model)
                << QObject::tr("No node was triggered!");
        }

        return triggeredNodes;
    }

}; // struct Impl

GraphExecutionModel::Impl::Synchronization GraphExecutionModel::Impl::s_sync{};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEXECMODEL_IMPL_H
