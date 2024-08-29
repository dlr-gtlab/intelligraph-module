#ifndef GT_INTELLI_GRAPHEXECMODEL_IMPL_H
#define GT_INTELLI_GRAPHEXECMODEL_IMPL_H

#include <intelli/graphexecmodel.h>
#include <intelli/graph.h>
#include <intelli/node.h>
#include <intelli/future.h>
#include <intelli/dynamicnode.h>
#include <intelli/nodeexecutor.h>

#include <intelli/private/utils.h>

#include <gt_utilities.h>

#include <gt_logging.h>

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
           QObject::tr("Failed to set node data") + ',';
};

inline QString getNodeDataError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("Failed to access node data") + ',';
};

inline QString evaluteNodeError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("Evaluate node: cannot evaluate node") + ',';
};

/// Helper struct to "hide" implementation details and template functions
struct GraphExecutionModel::Impl
{

    /// Wrapper to retrieve iterator type for `QHash` depending on `IsConst`
    template<bool IsConst>
    struct DataItemHelper;

    template<bool IsConst>
    struct PortDataItemHelper;

    using MutableDataItemHelper = DataItemHelper<false>;
    using ConstDataItemHelper = DataItemHelper<true>;

    /// Finds either all start (root) or end (leaf) nodes of the graph. This can
    /// be determined if there no ingoing/outgoing connections
    template<typename List>
    static inline void
    findStartAndEndNodes(Graph const& graph, PortType type, List& targetNodes)
    {
        auto& conModel = graph.localConnectionModel();
        for (auto& entry : conModel)
        {
            if (entry.ports(type).empty())
            {
                targetNodes.push_back(entry.node->uuid());
            }
        }

        // recursive
        for (auto* subgraph : graph.graphNodes())
        {
            findStartAndEndNodes(*subgraph, type, targetNodes);
        }
    }

    /// Finds all start/root nodes of the graph.
    template<typename List>
    static inline void
    findRootNodes(Graph const& graph, List& targetNodes)
    {
        findStartAndEndNodes(graph, PortType::In, targetNodes);
    }

    /// Finds all end/leaf nodes of the graph.
    template<typename List>
    static inline void
    findLeafNodes(Graph const& graph, List& targetNodes)
    {
        findStartAndEndNodes(graph, PortType::Out, targetNodes);
    }

    /// Returns the port type for the given entry and port vector
    template<typename E, typename P>
    static inline PortType
    portType(E& e, P& p)
    {
        if (&e.portsIn == &p) return PortType::In;
        if (&e.portsOut == &p) return PortType::Out;
        return PortType::NoType;
    }

    template<typename ExecModel>
    static inline auto
    findTargetNode(ExecModel& model, NodeUuid const& nodeUuid)
    {
        return std::find_if(model.m_targetNodes.begin(),
                            model.m_targetNodes.end(),
                            [nodeUuid](TargetNode const& target){
            return target.nodeUuid == nodeUuid;
        });
    }

    template<typename ExecModel>
    static inline auto
    findQueuedNode(ExecModel& model, NodeUuid const& nodeUuid)
    {
        return std::find(model.m_queuedNodes.begin(),
                         model.m_queuedNodes.end(),
                         nodeUuid);
    }

    static inline bool
    removeFromTargetNodes(GraphExecutionModel& model,
                          NodeUuid const& nodeUuid)
    {
        auto iter = findTargetNode(model, nodeUuid);
        if (iter != model.m_targetNodes.end())
        {
            model.m_targetNodes.erase(iter);
            return true;
        }
        return false;
    }

    static inline bool
    removeFromQueuedNodes(GraphExecutionModel& model, NodeUuid const& nodeUuid)
    {
        auto iter = findQueuedNode(model, nodeUuid);
        if (iter != model.m_queuedNodes.end())
        {
            model.m_queuedNodes.erase(iter);
            return true;
        }
        return false;
    }

    static inline bool
    removeFromTargetNodes(GraphExecutionModel& model,
                          NodeUuid const& nodeUuid,
                          NodeEvaluationType evalType)
    {
        auto iter = findTargetNode(model, nodeUuid);
        if (iter != model.m_targetNodes.end() && iter->evalType == evalType)
        {
            model.m_targetNodes.erase(iter);
            return true;
        }
        return false;
    }

    template<typename ExecModel>
    static inline auto
    remove(ExecModel& model, NodeUuid const& nodeUuid)
    {
        return std::find(model.m_queuedNodes.begin(),
                         model.m_queuedNodes.end(),
                         nodeUuid);
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

    /// Returns whether the node is evaluating
    static inline bool
    isNodeEvaluating(Node const& node)
    {
        return node.nodeFlags() & NodeFlag::Evaluating;
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
                           .arg(node->id())
                           .arg(node->caption());
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
        PortType type = PortType::NoType;

        auto* portEntry = item.entry->findPort(portId, &type);
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
        auto* graph = qobject_cast<Graph*>(model.sender());
        if (!graph)
        {
            if (makeError) gtError()
                    << makeError(model.graph())
                    << tr("graph node not found!");
            return {};
        }

        return findPortData(model, *graph, nodeId, type, portIdx, makeError);
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
            return entry->state == NodeEvalState::Outdated ||
                   entry->state == NodeEvalState::Invalid;
        }

        bool inputsValid() const
        {
            auto& conModel = execModel->graph().globalConnectionModel();
            auto* conData = connection_model::find(conModel, node->uuid());

            bool valid =
                std::all_of(entry->portsIn.begin(), entry->portsIn.end(),
                        [conData, this](PortDataItem const& entry){

                bool isConnected = connection_model::hasPredecessors(conData, entry.portId);
                bool isPortDataValid = entry.data.state == PortDataState::Valid;

                auto* port = node->port(entry.portId);
                bool hasRequiredData = port && (port->optional || entry.data.ptr);

                return (!isConnected || isPortDataValid) && hasRequiredData;
            });

            return valid;
        }

        bool isEvaluating() const
        {
            return isNodeEvaluating(*node);
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

    static inline bool
    invalidateNodeHelper(GraphExecutionModel& model,
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

        if (item.node->nodeEvalMode() != NodeEvalMode::ForwardInputsToOutputs)
        {
            return invalidateNodeHelper(model, nodeUuid, item);
        }

        // node is forwarding data from input to respective output
        item.entry->state = NodeEvalState::Outdated;
        emit model.nodeEvalStateChanged(nodeUuid, QPrivateSignal());

        PortType type = item.node->portType(portId);
        assert(type != PortType::NoType);

        bool success = false;

        switch (type)
        {
        // invalidate respective output port
        case PortType::In:
        {
            // TODO: check if correct
            PortIndex idx = item.node->portIndex(PortType::In, item->portId);
            if (idx < 0 || (size_t)item.entry->portsOut.size() <= idx)
            {
                break;
            }
            success = invalidateNodeHelper(model, nodeUuid, item.node->portId(PortType::Out, idx));
            break;
        }
        // invalidate all outgoing connections of this port only
        case PortType::Out:
        {
            auto& conModel = model.graph().globalConnectionModel();
            auto* conData  = connection_model::find(conModel, nodeUuid);
            success = connection_model::visitSuccessors(conData, portId,
                                                     [&model](auto& con){
                invalidateNodeHelper(model, con.node, con.port);
                return true;
            });
            break;
        }
        case PortType::NoType:
            break;
        }
        return success;
    }

    static inline bool
    invalidateNodeHelper(GraphExecutionModel& model,
                         NodeUuid const& nodeUuid,
                         MutableDataItemHelper item)
    {
        item->state = NodeEvalState::Outdated;

        INTELLI_LOG_SCOPE(model)
            << tr("invalidating node '%1' (%2)...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        auto finally = gt::finally([&model, &nodeUuid](){
            emit model.nodeEvalStateChanged(nodeUuid, QPrivateSignal());
        });

        removeFromQueuedNodes(model, nodeUuid);

        bool success = true;
        for (auto& port : item->portsOut)
        {
            if (port.data.state != PortDataState::Valid) continue;

            port.data.state = PortDataState::Outdated;

            // find and invalidate connected nodes
            auto* conData = connection_model::find(model.graph().globalConnectionModel(), nodeUuid);
            success &= connection_model::visitSuccessors(conData, port.portId,
                                                         [&model](auto& con){
                invalidateNodeHelper(model, con.node, con.port);
                return true;
            });
        }

        return success;
    }

    static inline FutureEvaluated
    evaluateGraph(GraphExecutionModel& model,
                  Graph const& graph,
                  NodeEvaluationType evalType)
    {
        assert(containsGraph(model, graph));

        FutureEvaluated future{model};

        QVarLengthArray<NodeUuid, PRE_ALLOC> targetNodes;
        findLeafNodes(graph, targetNodes);

        for (auto const& target : targetNodes)
        {
            NodeEvalState evalState = scheduleTargetNode(
                model, target, evalType
            );

            future.append(target, evalState);
            if (!future.detach()) return future;
        }

        return future;
    }

    static inline FutureEvaluated
    evaluateNode(GraphExecutionModel& model,
                 NodeUuid const& nodeUuid,
                 NodeEvaluationType evalType)
    {
        NodeEvalState evalState = scheduleTargetNode(
            model, nodeUuid, evalType
            );

        FutureEvaluated future{
            model, nodeUuid, evalState
        };
        return future;
    }

    static inline void
    unscheduleNode(GraphExecutionModel& model,
                   NodeUuid const& nodeUuid)
    {
        // TODO (find data not necessary)
        auto item = findData(model, nodeUuid);
        if (item) item->isPending = false;
    }

    static inline void
    unscheduleNodeRecursively(GraphExecutionModel& model,
                              NodeUuid const& nodeUuid)
    {
        unscheduleNode(model, nodeUuid);

        auto& conModel = model.graph().globalConnectionModel();
        auto* conData = connection_model::find(conModel, nodeUuid);

        // clear predecessors
        connection_model::visitPredeccessors(conData, [&model](auto& con){
            unscheduleNodeRecursively(model, con.node);
            return true;
        });
    }

    static inline void
    rescheduleTargetNodes(GraphExecutionModel& model)
    {
        if (model.m_targetNodes.empty()) return;

        INTELLI_LOG_SCOPE(model)
            << tr("rescheduling target nodes...");

        foreach (TargetNode const& target, model.m_targetNodes)
        {
            // only reschedule if target node is not running already
            switch (model.nodeEvalState(target.nodeUuid))
            {
            case NodeEvalState::Valid:
            case NodeEvalState::Evaluating:
                continue;
            default:
                break;
            }

            auto state = scheduleTargetNode(model, target.nodeUuid, target.evalType);
            if (state == NodeEvalState::Invalid)
            {
                emit model.nodeEvaluationFailed(target.nodeUuid, QPrivateSignal());
                emit model.graphStalled(QPrivateSignal());
            }
        }
    }

    static inline NodeEvalState
    scheduleTargetNode(GraphExecutionModel& model,
                       NodeUuid const& nodeUuid,
                       NodeEvaluationType evalType)
    {
        assert(!nodeUuid.isEmpty());

        auto item = findData(model, nodeUuid, evaluteNodeError);
        if (!item) return NodeEvalState::Invalid;

        INTELLI_LOG_SCOPE(model)
            << tr("setting target node '%1' (%2)...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        // register node as a target
        auto iter = findTargetNode(model, nodeUuid);
        if (iter != model.m_targetNodes.end())
        {
            INTELLI_LOG(model) << tr("node is already a target node!");

            if (iter->evalType == NodeEvaluationType::SingleShot)
            {
                iter->evalType = evalType;
            }
        }
        else
        {
            model.m_targetNodes.push_back({ nodeUuid, evalType });
        }

        auto state = queueNode(model, nodeUuid, item);
        switch (state)
        {
        case NodeEvalState::Invalid:
            removeFromTargetNodes(model, nodeUuid);
            break;
        case NodeEvalState::Valid:
            // node was evaluated -> remove from target nodes
            removeFromTargetNodes(model, nodeUuid, NodeEvaluationType::SingleShot);
            break;
        case NodeEvalState::Evaluating:
        case NodeEvalState::Outdated:
        case NodeEvalState::Paused:
            break;
        }
        return state;
    }

    static inline NodeEvalState
    scheduleDependencies(GraphExecutionModel& model,
                         NodeUuid const& nodeUuid,
                         MutableDataItemHelper item)
    {
        assert(item && !item.isReadyForEvaluation());

        if (item->isPending)
        {
            INTELLI_LOG_SCOPE(model)
                << tr("node '%1' (%2) is not ready for evaluation and was already checked!")
                       .arg(relativeNodePath(*item.node))
                       .arg(item.node->id());
            return NodeEvalState::Evaluating;
        }

        INTELLI_LOG_SCOPE(model)
            << tr("node '%1' (%2) is not ready for evaluation, checking dependencies...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        auto& conModel = model.graph().globalConnectionModel();

        auto* conData  = connection_model::find(conModel, nodeUuid);
        if (!conData)
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("node '%1' (%2) not found in connection model!")
                             .arg(relativeNodePath(*item.node))
                             .arg(item.node->id());
            return NodeEvalState::Invalid;
        }

        if (conData->predecessors.empty())
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("node '%1' (%2) is not ready and has no dependencies!")
                             .arg(relativeNodePath(*item.node))
                             .arg(item.node->id());
            return NodeEvalState::Invalid;
        }

        // iterate over all dependencies
        int validNodes = 0;
        for (auto& predecessor : conData->predecessors)
        {
            auto dependency = findData(model, predecessor.node);
            if (!dependency)
            {
                gtError() << evaluteNodeError(model.graph())
                          << tr("node dependency %1 not found!")
                                 .arg(predecessor.node);
                return NodeEvalState::Invalid;
            }

            if (model.nodeEvalState(predecessor.node) == NodeEvalState::Valid)
            {
                validNodes++;
            }

            auto state = queueNode(model, predecessor.node, dependency);
            if (state == NodeEvalState::Invalid)
            {
                // TODO
                gtDebug() << "DEPENDENCY" << dependency.node << "FAILED, from" << item.node;
                return NodeEvalState::Invalid;
            }
        }

        // all nodes are valid yet the target node is not ready
        // -> something went wrong
        if (validNodes == conData->predecessors.size())
        {
            // TODO
            gtDebug() << "DEPENDENCIES ARE READY, NOT TARGET" << item.node;
            return NodeEvalState::Invalid;
        }

        item->isPending = true;

        return NodeEvalState::Outdated;
    }

    static inline NodeEvalState
    queueNode(GraphExecutionModel& model,
              NodeUuid const& nodeUuid,
              MutableDataItemHelper item)
    {        
        assert(item);

        INTELLI_LOG_SCOPE(model)
            << tr("attempting to queue node '%1' (%2)!")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        if (item.isEvaluating())
        {
            INTELLI_LOG(model)
                << tr("node is already evaluating!");
            return NodeEvalState::Evaluating;
        }

        if (item.isEvaluated())
        {
            INTELLI_LOG(model)
                << tr("node is already evaluated!");
            assert (model.nodeEvalState(nodeUuid) == NodeEvalState::Valid);
            return NodeEvalState::Valid;
        }

        auto iter = findQueuedNode(model, nodeUuid);
        if (iter != model.m_queuedNodes.end())
        {
            INTELLI_LOG(model)
                << tr("node is already queued!");
            return NodeEvalState::Evaluating;
        }

        // evaluate dependencies
        if (!item.isReadyForEvaluation())
        {
            auto state = scheduleDependencies(model, nodeUuid, item);
            return state;
        }

        item->isPending = true;

        int idx = model.m_queuedNodes.size();
        model.m_queuedNodes.push_back(nodeUuid);

        INTELLI_LOG(model)
            << tr("node '%1' (%2) queued!")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        auto state = tryEvaluatingNode(model, item, idx);
        switch (state)
        {
        case NodeEvalState::Paused:
            return NodeEvalState::Evaluating;
        case NodeEvalState::Outdated:
            return NodeEvalState::Invalid;
        case NodeEvalState::Evaluating:
        case NodeEvalState::Valid:
        case NodeEvalState::Invalid:
            break;
        }
        return state;
    }

    static inline NodeEvalState
    tryEvaluatingNode(GraphExecutionModel& model,
                      MutableDataItemHelper item,
                      int idx)
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

        // TODO: add proper support for exlusive nodes
        auto containsExclusiveNodes = false;

        // an exclusive node has to be evaluated separatly to all other nodes
        if (containsExclusiveNodes)
        {
            INTELLI_LOG(model)
                << tr("executor contains exclusive nodes!");
            return NodeEvalState::Paused;
        }

        bool isExclusive =
            (item.node->nodeEvalMode() == NodeEvalMode::ExclusiveBlocking ||
             item.node->nodeEvalMode() == NodeEvalMode::ExclusiveDetached);

        if (isExclusive)
        {
            INTELLI_LOG(model)
                << tr("node is exclusive and must wait for others to finish!");
            return NodeEvalState::Paused;
        }

        bool isBlocking =
            item.node->nodeEvalMode() == NodeEvalMode::ForwardInputsToOutputs ||
            item.node->nodeEvalMode() == NodeEvalMode::ExclusiveBlocking ||
            item.node->nodeEvalMode() == NodeEvalMode::Blocking;

        INTELLI_LOG(model)
            << tr("triggering node evaluation...");

        // disconnect old signal if still present
        auto disconnect = [exec = &model, node = item.node](){
            QObject::disconnect(node, &Node::computingFinished,
                                exec, &GraphExecutionModel::onNodeEvaluatedHelper);
        };
        disconnect();

        // subscribe to when node finished its execution
        QObject::connect(item.node, &Node::computingFinished,
                         &model, &GraphExecutionModel::onNodeEvaluatedHelper,
                         Qt::DirectConnection);

        assert(model.m_queuedNodes.size() > idx);
        assert(model.m_queuedNodes.at(idx) == item.node->uuid());
        model.m_queuedNodes.remove(idx);

        item->state = NodeEvalState::Evaluating;
        item->isPending = false;

        // trigger node evaluation
        if (!exec::triggerNodeEvaluation(*item.node, model))
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("node execution failed!");

            disconnect();
            item->state = NodeEvalState::Invalid;
            emit model.nodeEvalStateChanged(item.node->uuid(), QPrivateSignal());

            auto& conModel = model.graph().globalConnectionModel();
            auto* conData = connection_model::find(conModel, item.node->uuid());
            assert(conData);
            for (auto& successor : conData->successors)
            {
                auto item = findData(model, successor.node);
                item->state = NodeEvalState::Invalid;
                emit model.nodeEvalStateChanged(successor.node, QPrivateSignal());
            }

            return NodeEvalState::Invalid;
        }

//        if (isBlocking)
//        {
//            item->state = NodeEvalState::Valid;
//            emit model.nodeEvalStateChanged(item.node->uuid(), QPrivateSignal());
//            return NodeEvalState::Valid;
//        }

        return item->state;
    }

    static inline bool
    evaluateNextInQueue(GraphExecutionModel& model)
    {
        // do not evaluate if graph is currently being modified
        if (model.isBeingModified())
        {
            INTELLI_LOG(model)
                << tr("model is being modified!");
            return false;
        }

        bool scheduledNode = false;

        // for each node in queue
        for (int idx = 0; idx <= model.m_queuedNodes.size() - 1; ++idx)
        {
            auto const& nextNodeUuid = model.m_queuedNodes.at(idx);

            auto item = findData(model, nextNodeUuid);
            if (!item)
            {
                model.m_queuedNodes.remove(idx--);
                continue;
            }

            auto state = tryEvaluatingNode(model, item, idx);
            switch (state)
            {
            case NodeEvalState::Evaluating:
                scheduledNode = true;
                break;
            case NodeEvalState::Outdated:
                queueNode(model, nextNodeUuid, item);
                break;
            case NodeEvalState::Valid:
            case NodeEvalState::Invalid:
            case NodeEvalState::Paused:
                break;
            }
        }

        return scheduledNode;
    }

}; // struct Impl

} // namespace intelli


#endif // GT_INTELLI_GRAPHEXECMODEL_IMPL_H
