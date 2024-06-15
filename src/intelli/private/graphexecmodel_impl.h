#ifndef GT_INTELLI_GRAPHEXECMODEL_IMPL_H
#define GT_INTELLI_GRAPHEXECMODEL_IMPL_H

#include <intelli/graphexecmodel.h>
#include <intelli/graph.h>
#include <intelli/node.h>
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
    static inline QVarLengthArray<NodeUuid, PRE_ALLOC>
    findStartAndEndNodes(Graph const& graph, PortType type)
    {
        QVarLengthArray<NodeUuid, PRE_ALLOC> targetNodes;

        auto const& nodes = graph.nodes();
        for (auto const* node : nodes)
        {
            if (graph.findConnections(node->id(), type).empty())
            {
                targetNodes.push_back(node->uuid());
            }
        }
        return targetNodes;
    }

    /// Finds all start/root nodes of the graph.
    static inline auto
    findRootNodes(Graph const& graph)
    {
        return findStartAndEndNodes(graph, PortType::In);
    }

    /// Finds all end/leaf nodes of the graph.
    static inline auto
    findLeafNodes(Graph const& graph)
    {
        return findStartAndEndNodes(graph, PortType::Out);
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
        const_t<IsConst, GraphExecutionModel>* model = {};
        get_iterator_t<IsConst, GraphDataModel> entry = {};
        const_t<IsConst, Node>* node = {};

        operator bool() const { return model; }

        bool requiresReevaluation() const
        {
            return entry->state == DataState::RequiresReevaluation ||
                   entry->state == DataState::FailedEvaluation;
        }

        bool inputsValid(bool recurisve = true) const
        {
            auto graph = qobject_cast<Graph const*>(node->parent());
            assert(graph);

            if (recurisve)
            switch (entry->nodeType)
            {
            case NodeType::GroupInput:
                // check inputs of parent graph
                {
                    auto* graph = qobject_cast<const_t<IsConst, Graph>*>(node->parent());
                    if (!graph) return false;

                    auto item = findData(*model, graph->uuid());
                    if (!item) return false;

                    return item.inputsValid(false);
                }
            case NodeType::Group:
                // check outputs of subgraph
                {
                    auto* graph = qobject_cast<const_t<IsConst, Graph>*>(node);
                    if (!graph) return false;

                    auto* output = graph->outputNode();
                    if (!output) return false;

                    auto item = findData(*model, output->uuid());
                    if (!item || !item.inputsValid() || !item.isEvaluated()) return false;

                    break;
                }
            case NodeType::GroupOutput:
            case NodeType::Normal:
                break;
            }

            bool valid =
                std::all_of(entry->portsIn.begin(), entry->portsIn.end(),
                            [graph, this](PortDataItem const& entry){
                auto const& cons = graph->findConnections(node->id(), entry.id);
                auto* port = node->port(entry.id);

                bool hasConnections = !cons.empty();
                bool isPortDataValid = entry.data.state == PortDataState::Valid;
                bool hasPortRequiredData = port && (port->optional || entry.data.ptr);

                return (!hasConnections || isPortDataValid) && hasPortRequiredData;
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
                   (entry->state == DataState::Evaluated ||
                    entry->state == DataState::FailedEvaluation);
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
        auto item = Impl::findPortData(model, nodeUuid, portId);
        if (!item) return false;

        INTELLI_LOG(model)
            << tr("invalidating node '%1' (%2), port %4...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id())
                   .arg(portId);

        item->data.state = PortDataState::Outdated;

        switch (item.entry->nodeType)
        {
        case NodeType::Group:
        {
            auto* graph = qobject_cast<Graph*>(item.node);
            if (!graph) return false;
            auto* input = graph->inputNode();
            if (!input) return false;

            PortId inputPortId = input->portId(PortType::Out, graph->portIndex(PortType::In, portId));
            Impl::invalidateNodeHelper(model, input->uuid(), inputPortId);

            break;
        }
        case NodeType::GroupInput:
        {
            item.entry->state = DataState::RequiresReevaluation;

            emit model.nodeEvalStateChanged(nodeUuid, QPrivateSignal());

            auto* graph = qobject_cast<Graph*>(item.node->parent());
            if (!graph) return false;

            bool success = true;
            auto const& connections = graph->findConnections(item.node->id(), portId);
            for (ConnectionId conId : connections)
            {
                Node const* inNode = graph->findNode(conId.inNodeId);
                if (!inNode) { success = false; continue; }
                success &= Impl::invalidateNodeHelper(model, inNode->uuid(), conId.inPort);
            }

            return success;
        }
        case NodeType::GroupOutput:
        case NodeType::Normal:
            break;
        }

        return Impl::invalidateNodeHelper(model, nodeUuid, item);
    }

    static inline bool
    invalidateNodeHelper(GraphExecutionModel& model,
                         NodeUuid const& nodeUuid,
                         MutableDataItemHelper item)
    {
        item->state = DataState::RequiresReevaluation;

        INTELLI_LOG_SCOPE(model)
            << tr("invalidating node '%1' (%2)...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        auto finally = gt::finally([&model, &nodeUuid](){
            emit model.nodeEvalStateChanged(nodeUuid, QPrivateSignal());
        });

        auto* node = model.graph().findNodeByUuid(nodeUuid);
        if (!node) return false;
        auto* graph = qobject_cast<Graph*>(node->parent());
        if (!graph) return false;

        bool success = true;

        switch (item->nodeType)
        {
        case NodeType::GroupOutput:
            success &= model.invalidateNode(graph->uuid());
            break;
        case NodeType::Group:
        case NodeType::GroupInput:
        case NodeType::Normal:
            break;
        }

        for (auto& port : item->portsOut)
        {
            if (port.data.state != PortDataState::Valid) continue;

            port.data.state = PortDataState::Outdated;

            // item connected nodes and invalidate them aswell
            auto const& connections = graph->findConnections(node->id(), port.id);
            for (ConnectionId conId : connections)
            {
                Node const* inNode = graph->findNode(conId.inNodeId);
                if (!inNode) { success = false; continue; }
                success &= Impl::invalidateNodeHelper(model, inNode->uuid(), conId.inPort);
            }
        }

        return success;
    }

    static inline FutureEvaluated
    evaluateNode(GraphExecutionModel& model,
                NodeUuid const& nodeUuid,
                NodeEvaluationType evalType)
    {
        NodeEvalState evalState = Impl::scheduleTargetNode(
            model, nodeUuid, evalType
        );

        FutureEvaluated future{
            model, nodeUuid, evalState
        };
        return future;
    }

    static inline NodeEvalState
    scheduleTargetNode(GraphExecutionModel& model,
                       NodeUuid const& nodeUuid,
                       NodeEvaluationType evalType)
    {
        assert(!nodeUuid.isEmpty());

        auto item = Impl::findData(model, nodeUuid, evaluteNodeError);
        if (!item) return NodeEvalState::Invalid;

        INTELLI_LOG_SCOPE(model)
            << tr("setting target node '%1' (%2)...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        // target node
        auto iter = std::find_if(model.m_targetNodes.begin(),
                                 model.m_targetNodes.end(),
                                 [&nodeUuid](TargetNode const& node){
            return node.nodeUuid == nodeUuid;
        });

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

        auto state = scheduleNode(model, nodeUuid, item);
        if (state == NodeEvalState::Invalid)
        {
            auto iter = std::find_if(model.m_targetNodes.begin(),
                                     model.m_targetNodes.end(),
                                     [&nodeUuid](TargetNode const& node){
                return node.nodeUuid == nodeUuid;
            });
            assert(iter != model.m_targetNodes.end());
            model.m_targetNodes.erase(iter);
        }
        return state;
    }

    static inline NodeEvalState
    scheduleNode(GraphExecutionModel& model,
                 NodeUuid const& nodeUuid,
                 MutableDataItemHelper item)
    {
        assert(item);

        //    INTELLI_LOG(model) << tr("scheduling node %1 (%2:%3)...")
        //                              .arg(nodeUuid)
        //                              .arg(item->nodeId, 2)
        //                              .arg(item.node->caption());
        //    INTELLI_LOG_SCOPE(model)

        //    // pending node
        //    auto iter = std::find(model.m_pendingNodes.begin(),
        //                          model.m_pendingNodes.end(),
        //                          nodeUuid);
        //    if (iter != model.m_pendingNodes.end())
        //    {
        //        INTELLI_LOG(model)
        //            << tr("node is already scheduled!");
        //        return NodeEvalState::Evaluating;
        //    }

        //    model.m_pendingNodes.push_back(nodeUuid);

        return queueNode(model, nodeUuid, item);
    }

    static inline NodeEvalState
    scheduleDependencies(GraphExecutionModel& model,
                         NodeUuid const& nodeUuid,
                         MutableDataItemHelper item)
    {
        assert(item && !item.isReadyForEvaluation());

        auto* graph = qobject_cast<Graph*>(item.node->parent());
        if (!graph)
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("failed to access parent graph of node '%1' (%2)!")
                             .arg(relativeNodePath(*item.node))
                             .arg(item.node->id());
            return NodeEvalState::Invalid;
        }

        INTELLI_LOG_SCOPE(model)
            << tr("node '%1' (%2) is not ready for evaluation, checking dependencies...")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        QVarLengthArray<NodeUuid, PRE_ALLOC> dependencyUuids;

        auto const appendUuids = [&dependencyUuids](Graph const& graph,
                                                    auto const& dependencyIds){
            std::transform(dependencyIds.begin(), dependencyIds.end(),
                           std::back_inserter(dependencyUuids),
                           [&graph](NodeId nodeId){
                Node const* node = graph.findNode(nodeId);
                assert(node);
                return node->uuid();
            });
        };

        appendUuids(*graph, graph->findConnectedNodes(item->nodeId, PortType::In));

        // find special dependencies...
        switch (item->nodeType)
        {
        case NodeType::GroupInput:
        // find inputs of parent graph
        {
            auto* parentGraph = qobject_cast<Graph*>(graph->parent());
            if (!parentGraph)
            {
                gtError() << evaluteNodeError(model.graph())
                          << tr("failed to access root graph of node '%1' (%2)!")
                                 .arg(relativeNodePath(*item.node))
                                 .arg(item.node->id());
                return NodeEvalState::Invalid;
            }
            appendUuids(*parentGraph, parentGraph->findConnectedNodes(graph->id(), PortType::In));
            break;
        }
        case NodeType::Group:
        // find outputs of subgraph
        {
            graph = qobject_cast<Graph*>(item.node);
            if (!graph)
            {
                gtError() << evaluteNodeError(model.graph())
                          << tr("failed to access graph '%1' (%2)!")
                                 .arg(relativeNodePath(*item.node))
                                 .arg(item.node->id());
                return NodeEvalState::Invalid;
            }
            auto* output = graph->outputNode();
            if (!output)
            {
                gtError() << evaluteNodeError(model.graph())
                          << tr("failed to access output node of graph '%1' (%2)!")
                                 .arg(relativeNodePath(*item.node))
                                 .arg(item.node->id());
                return NodeEvalState::Invalid;
            }
            appendUuids(*graph, std::array<NodeId, 1>{output->id()});
            break;
        }
        case NodeType::GroupOutput:
        case NodeType::Normal:
            break;
        }

        if (dependencyUuids.empty())
        {
            gtError() << evaluteNodeError(*graph)
                      << tr("node '%1' (%2) is not ready and has no dependencies!")
                             .arg(relativeNodePath(*item.node))
                             .arg(item.node->id());
            return NodeEvalState::Invalid;
        }

        int validNodes = 0;
        for (NodeUuid const& dependencyUuid : dependencyUuids)
        {
            assert (!dependencyUuid.isEmpty());

            auto dependency = findData(model, dependencyUuid);
            if (!dependency)
            {
                gtError() << evaluteNodeError(*graph)
                          << tr("node dependency %1 not found!")
                                 .arg(dependencyUuid);
                return NodeEvalState::Invalid;
            }

            if (model.nodeEvalState(dependencyUuid) == NodeEvalState::Valid)
            {
                validNodes++;
            }

            auto state = scheduleNode(model, dependencyUuid, dependency);
            if (state == NodeEvalState::Invalid)
            {
                gtDebug() << "DEPENDENCY" << dependency.node << "FAILED, from" << item.node;
                return NodeEvalState::Invalid;
            }
        }

        // all nodes are valid yet the target node is not ready
        // -> something went wrong
        if (validNodes == dependencyUuids.size())
        {
            gtDebug() << "DEPENDENCIES ARE READY, NOT TARGET" << item.node;
            return NodeEvalState::Invalid;
        }

        return NodeEvalState::Outdated;
    }

    static inline NodeEvalState
    queueNode(GraphExecutionModel& model,
              NodeUuid const& nodeUuid,
              MutableDataItemHelper item)
    {
        INTELLI_LOG_SCOPE(model)
            << tr("attempting to queue node '%1' (%2)!")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        assert(item);

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
            // TODO: should we check if the node is valid or outdated?
            return NodeEvalState::Valid;
        }

        auto iter = std::find(model.m_queuedNodes.begin(),
                              model.m_queuedNodes.end(),
                              nodeUuid);
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

        model.m_queuedNodes.push_back(nodeUuid);

        INTELLI_LOG(model)
            << tr("node '%1' (%2) queued!")
                   .arg(relativeNodePath(*item.node))
                   .arg(item.node->id());

        auto state = tryEvaluatingNode(model, item);
        switch (state)
        {
        case NodeEvalState::Evaluating:
        case NodeEvalState::Valid:
        case NodeEvalState::Invalid:
            // node is evaluating or was evaluated
            model.m_queuedNodes.removeLast();
            break;
        case NodeEvalState::Paused:
            return NodeEvalState::Evaluating;
        case NodeEvalState::Outdated:
            model.m_queuedNodes.removeLast();
            return NodeEvalState::Invalid;
        }
        return state;
    }

    static inline NodeEvalState
    tryEvaluatingNode(GraphExecutionModel& model,
                      MutableDataItemHelper item)
    {
        assert(item);
        if (!item.isReadyForEvaluation())
        {
            return NodeEvalState::Outdated;
        }

        // TODO: add proper support for exlusive nodes
        auto containsExclusiveNodes = false;

        // an exclusive node has to be evaluated separatly to all other nodes
        if (containsExclusiveNodes)
        {
            INTELLI_LOG(model)
                << tr("executor contains exclusive nodes");
            return NodeEvalState::Paused;
        }

        bool isExclusive =
            item.node->nodeEvalMode() == NodeEvalMode::ExclusiveBlocking ||
            item.node->nodeEvalMode() == NodeEvalMode::ExclusiveDetached;

        if (isExclusive)
        {
            INTELLI_LOG(model)
                << tr("node is exclusive and must wait for others to finish!");
            return NodeEvalState::Paused;
        }

        bool isBlocking =
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

        // mark node as evaluated
        item->state = DataState::Evaluated;

        if (!exec::triggerNodeEvaluation(*item.node, model))
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("node execution failed!");

            disconnect();

            item->state = DataState::FailedEvaluation;

            emit model.nodeEvalStateChanged(item.node->uuid(), QPrivateSignal());

            return NodeEvalState::Invalid;
        }

        emit model.nodeEvalStateChanged(item.node->uuid(), QPrivateSignal());

        return (isBlocking) ? NodeEvalState::Valid : NodeEvalState::Evaluating;
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

            auto state = tryEvaluatingNode(model, item);
            switch (state)
            {
            case NodeEvalState::Evaluating:
                scheduledNode = true;
                break;
                // fall through expected
            case NodeEvalState::Valid:
            case NodeEvalState::Invalid:
                // node is evaluating or was evaluated
                model.m_queuedNodes.remove(idx--);
                break;
            case NodeEvalState::Paused:
                // node was not yet scheduled
                break;
            case NodeEvalState::Outdated:
                scheduleNode(model, nextNodeUuid, item);
                break;
            }
        }

        return scheduledNode;
    }

}; // struct Impl

} // namespace intelli


#endif // GT_INTELLI_GRAPHEXECMODEL_IMPL_H
