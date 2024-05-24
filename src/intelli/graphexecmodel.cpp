/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/graphexecmodel.h>

#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/connection.h"
#include "intelli/nodeexecutor.h"

#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include <intelli/private/utils.h>

#include <gt_utilities.h>
#include <gt_exceptions.h>
#include <gt_eventloop.h>

#include <gt_logging.h>

using namespace intelli;
using namespace intelli::data_model;

#ifdef GT_INTELLI_DEBUG_NODE_EXEC

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

#define INTELLI_LOG_IMPL(MODEL, INDENT) \
    gtTrace().verbose() \
        << (MODEL).graph().objectName() + QChar{':'} + makeIndentation(INDENT)
#define INTELLI_LOG_SCOPE(MODEL) \
    auto undo_indentation__ = gt::finally([&](){ getIndentation((MODEL))--; }); \
    INTELLI_LOG_IMPL(MODEL, getIndentation((MODEL))++)
#define INTELLI_LOG(MODEL) \
    INTELLI_LOG_IMPL(MODEL, getIndentation((MODEL)))
#else
#define INTELLI_LOG_SCOPE(MODEL) if (false) gtTrace()
#define INTELLI_LOG(MODEL) if (false) gtTrace()
#endif

using MakeErrorFunction = QString(*)(Graph const&);

QString setNodeDataError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("Failed to set node data") + ',';
};

QString getNodeDataError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("Failed to access node data") + ',';
};

QString evaluteNodeError(Graph const& graph)
{
    return graph.objectName() + QStringLiteral(": ") +
           QObject::tr("Evaluate node: cannot evaluate node") + ',';
};

struct GraphExecutionModel::Impl
{

/// retrieve iterator type for `QHash` depending on `IsConst`
template<bool IsConst>
struct data_iterator;
template<>
struct data_iterator<true> { using type = GraphDataModel::const_iterator; };
template<>
struct data_iterator<false> { using type = GraphDataModel::iterator; };

/// wrapper to retrieve iterator type for `QHash` depending on `IsConst`
template<bool IsConst>
using data_iterator_t = typename data_iterator<IsConst>::type;

template<bool IsConst>
struct DataItemHelper;

using MutableDataItemHelper = DataItemHelper<false>;
using ConstDataItemHelper = DataItemHelper<true>;

template<bool IsConst>
struct PortDataItemHelper;

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

/// returns the port type for the given entry and port vector
template<typename E, typename P>
static inline PortType
portType(E& e, P& p)
{
    if (&e.portsIn == &p) return PortType::In;
    if (&e.portsOut == &p) return PortType::Out;
    return PortType::NoType;
}

/// returns whether the node is evaluating
static inline bool
isNodeEvaluating(Node const& node)
{
    return node.nodeFlags() & NodeFlag::Evaluating;;
}

template<typename ExecModel>
static inline DataItemHelper<is_const<ExecModel>::value>
findData(ExecModel& model,
         NodeUuid nodeUuid,
         MakeErrorFunction makeError = {})
{
    auto& graph = model.graph();
    Node* node = graph.findNodeByUuid(nodeUuid);
    if (!node)
    {
        if (makeError) gtError()
                << makeError(graph)
                << tr("node %1 not found!")
                       .arg(nodeUuid);
        return {};
    }

    auto iter = model.m_data.find(node->uuid());
    if (iter == model.m_data.end())
    {
        if (makeError) gtError()
                << makeError(graph)
                << tr("entry for node %1 (%2:%3) not found!")
                       .arg(nodeUuid)
                       .arg(node->id(), 2)
                       .arg(node->caption());
        return {};
    }

    return { &model, iter, node };
}

template<typename ExecModel>
static inline DataItemHelper<is_const<ExecModel>::value>
findData(ExecModel& model,
         Graph& graph,
         NodeId nodeId,
         MakeErrorFunction makeError = {})
{
    Node* node = graph.findNode(nodeId);
    if (!node)
    {
        if (makeError) gtError()
                << makeError(graph)
                << tr("node %1:%2 not found!")
                       .arg(graph.uuid())
                       .arg(nodeId);
        return {};
    }

    return findData(model, node->uuid(), makeError);
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
             Graph& graph,
             NodeId nodeId,
             PortType type,
             PortIndex portIdx,
             MakeErrorFunction makeError = {})
{
    auto item = findData(model, graph, nodeId, makeError);
    if (!item) return {};

    PortId portId = item.node->portId(type, portIdx);

    auto* portEntry = item.entry->findPort(portId);
    if (!portEntry)
    {
        if (makeError) gtError()
                << makeError(model.graph())
                << tr("port entry for node %1:%2 (%3, port %4) not found!")
                       .arg(graph.uuid())
                       .arg(nodeId)
                       .arg(toString(type))
                       .arg(portId);
        return {};
    }

    return { item, portEntry, type };
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
    auto entry = model.m_data.find(nodeUuid);
    if (entry == model.m_data.end())
    {
        if (makeError) gtError()
                << makeError(model.graph())
                << tr("node entry for node %1 not found!")
                       .arg(nodeUuid);
        return {};
    }

    auto* node = model.graph().findNodeByUuid(nodeUuid);

    PortType type = PortType::NoType;
    auto* port = entry->findPort(portId, &type);

    return { {&model, entry, node}, port, type };
}

template<bool IsConst>
struct DataItemHelper
{
    const_t<IsConst, GraphExecutionModel>* model = {};
    data_iterator_t<IsConst> entry = {};
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

            auto* output = graph->outputProvider();
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

            INTELLI_LOG(*model) << "ARE INPUTS VALID?" << entry.id
                                << "( hasConnections:" << hasConnections
                                << "|| isPortDataValid" << isPortDataValid
                                << ") && hasPortRequiredData" << hasPortRequiredData
                                << "- is node evaluating?" << isNodeEvaluating(*node);

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

    data_iterator_t<IsConst> operator->() { return entry; }
    data_iterator_t<IsConst> operator->() const { return entry; }
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
                     MutableDataItemHelper item)
{
    item->state = DataState::RequiresReevaluation;

    INTELLI_LOG_SCOPE(model)
        << tr("invalidating node %1 (%2:%3)")
               .arg(nodeUuid)
               .arg(item->nodeId, 2)
               .arg(item.node->caption());

    bool success = true;
    for (auto& port : item->portsOut)
    {
        if (port.data.state != PortDataState::Valid) continue;

        port.data.state = PortDataState::Outdated;

        // item connected nodes and invalidate them aswell
        auto* node = model.graph().findNodeByUuid(nodeUuid);
        if (!node) { success = false; continue; }
        auto* graph = qobject_cast<Graph*>(node->parent());
        if (!graph) { success = false; continue; }

        for (auto& targetId : graph->findConnectedNodes(node->id(), PortType::Out))
        {
        Node const* inNode = graph->findNode(targetId);
        if (!inNode) { success = false; continue; }
        success &= model.invalidateNode(inNode->uuid());
        }
    }

    emit model.nodeEvalStateChanged(nodeUuid, QPrivateSignal());

    return success;
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
        << tr("setting target node %1 (%2:%3)...")
               .arg(nodeUuid)
               .arg(item->nodeId, 2)
               .arg(item.node->caption());

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
                  << tr("failed to access parent graph of node %1 (%2:%3)!")
                         .arg(nodeUuid)
                         .arg(item->nodeId, 2)
                         .arg(item.node->caption());
        return NodeEvalState::Invalid;
    }

    INTELLI_LOG_SCOPE(model)
        << tr("node %1 (%2:%3) is not ready for evaluation, checking dependencies...")
               .arg(item.node->uuid())
               .arg(item.node->id(), 2)
               .arg(item.node->caption());

    QVarLengthArray<NodeUuid, PRE_ALLOC> dependencyUuids;

    auto const appendUuids = [&dependencyUuids](Graph const& graph, auto const& dependencyIds){
        std::transform(dependencyIds.begin(), dependencyIds.end(),
                       std::back_inserter(dependencyUuids),
                       [&graph](NodeId nodeId){
            auto* node = graph.findNode(nodeId);
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
                      << tr("failed to access root graph of node %1 (%2:%3)!")
                             .arg(graph->uuid())
                             .arg(graph->id(), 2)
                             .arg(graph->caption());
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
                      << tr("failed to access graph %1 (%2:%3)!")
                             .arg(item.node->uuid())
                             .arg(item.node->id(), 2)
                             .arg(item.node->caption());
            return NodeEvalState::Invalid;
        }
        auto* output = graph->outputProvider();
        if (!output)
        {
            gtError() << evaluteNodeError(model.graph())
                      << tr("failed to access output node of graph %1 (%2:%3)!")
                             .arg(item.node->uuid())
                             .arg(item.node->id(), 2)
                             .arg(item.node->caption());
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
                  << tr("node %1 (%2:%3) is not ready and has no dependencies!")
                         .arg(nodeUuid)
                         .arg(item->nodeId, 2)
                         .arg(item.node->caption());
        return NodeEvalState::Invalid;
    }

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

        auto state = scheduleNode(model, dependencyUuid, dependency);
        if (state == NodeEvalState::Invalid)
        {
            return NodeEvalState::Invalid;
        }
    }

    return NodeEvalState::Outdated;
}

static inline NodeEvalState
queueNode(GraphExecutionModel& model,
          NodeUuid const& nodeUuid,
          MutableDataItemHelper item)
{
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
        << tr("node %1 (%2:%3) queued!")
               .arg(nodeUuid)
               .arg(item->nodeId, 2)
               .arg(item.node->caption());

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
            model.m_queuedNodes.remove(idx--);
            scheduleNode(model, nextNodeUuid, item);
            break;
        }
    }

    return scheduledNode;
}

}; // struct Impl

GraphExecutionModel::GraphExecutionModel(Graph& graph) :
    m_graph(&graph)
{
    if (graph.parentGraph())
    {
        gtError() << tr("A graph execution model should only be added to the root Graph!");
        m_modificationCount++; // deactive this exec model
    }

    if (auto* exec = graph.findDirectChild<GraphExecutionModel*>())
    if (exec != this)
    {
        gtError() << tr("The graph %1 already has a graph execution model associated!")
                        .arg(graph.objectName());
    }

    setObjectName(QStringLiteral("__exec_model"));
    setParent(&graph);

    reset();
}

GraphExecutionModel*
GraphExecutionModel::accessExecModel(Graph& graph)
{
    if (graph.parentGraph())
    {
        auto root = graph.findRoot<Graph*>(&graph);
        if (!root) return {};

        return root->findDirectChild<GraphExecutionModel*>();
    }

    return graph.findDirectChild<GraphExecutionModel*>();
}

GraphExecutionModel const*
GraphExecutionModel::accessExecModel(const Graph& graph)
{
    return accessExecModel(const_cast<Graph&>(graph));
}

Graph&
GraphExecutionModel::graph()
{
    assert(m_graph);
    return *m_graph;
}

Graph const&
GraphExecutionModel::graph() const
{
    return const_cast<GraphExecutionModel*>(this)->graph();
}

void
GraphExecutionModel::reset()
{
    beginModification();

    beginReset();
    endReset();

    endModification();
}

NodeEvalState
GraphExecutionModel::nodeEvalState(NodeUuid const& nodeUuid)
{
    auto find = Impl::findData(*this, nodeUuid);
    if (!find) return NodeEvalState::Invalid;

    // paused
    if (!find.node->isActive())
    {
        return NodeEvalState::Paused;
    }

    // evaluating
    if (find.isEvaluating())
    {
        return NodeEvalState::Evaluating;
    }

    if (find.isEvaluated())
    {
        assert(!find.requiresReevaluation());

        if (find->state == DataState::FailedEvaluation)
        {
            return NodeEvalState::Invalid;
        }
        if (find.node->ports(PortType::Out).empty() && !find.inputsValid())
        {
            return NodeEvalState::Invalid;
        }

        return NodeEvalState::Valid;
    }

    return NodeEvalState::Outdated;
}

void
GraphExecutionModel::beginReset()
{
    m_autoEvaluateGraph = false;

    auto iter = m_data.keyBegin();
    auto end  = m_data.keyEnd();
    for (; iter != end; ++iter)
    {
        auto& entry = *m_data.find(*iter);
        entry.state = DataState::RequiresReevaluation;
        for (auto& entry : entry.portsIn ) entry.data.state = PortDataState::Outdated;
        for (auto& entry : entry.portsOut) entry.data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_targetNodes.clear();
//    m_pendingNodes.clear();
    m_queuedNodes.clear();
    m_data.clear();

    Graph& graph = this->graph();
    setupConnections(graph);

    auto const& nodes = graph.nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }
}

void
GraphExecutionModel::beginModification()
{
    INTELLI_LOG(*this)
        << tr("BEGIN MODIFICIATION...")
        << m_modificationCount;

    assert(m_modificationCount >= 0);
    m_modificationCount++;
}

void
GraphExecutionModel::endModification()
{
    m_modificationCount--;
    assert(m_modificationCount >= 0);

    INTELLI_LOG(*this)
        << /*Impl::graphName(*this) <<*/ tr("...END MODIFICATION")
        << m_modificationCount;

    // TODO: explicitly reschedule the graph?
    if (m_modificationCount != 0) return;
}

bool
GraphExecutionModel::isBeingModified() const
{
    return m_modificationCount > 0;
}

void
GraphExecutionModel::setupConnections(Graph& graph)
{
    disconnect(&graph);

    connect(&graph, &Graph::nodeAppended,
            this, &GraphExecutionModel::onNodeAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodeAboutToBeDeleted,
            this, [this, g = &graph](NodeId nodeId){
                onNodeDeleted(g, nodeId);
            }, Qt::DirectConnection);
    connect(&graph, &Graph::connectionAppended,
            this, &GraphExecutionModel::onConnectedionAppended,
            Qt::DirectConnection);
    connect(&graph, &Graph::connectionDeleted,
            this, &GraphExecutionModel::onConnectionDeleted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodePortInserted,
            this, &GraphExecutionModel::onNodePortInserted,
            Qt::DirectConnection);
    connect(&graph, &Graph::nodePortAboutToBeDeleted,
            this, &GraphExecutionModel::onNodePortAboutToBeDeleted,
            Qt::DirectConnection);
    connect(&graph, &Graph::beginModification,
            this, &GraphExecutionModel::onBeginGraphModification,
            Qt::DirectConnection);
    connect(&graph, &Graph::endModification,
            this, &GraphExecutionModel::onEndGraphModification,
            Qt::DirectConnection);
}

bool
GraphExecutionModel::isAutoEvaluatingNode(NodeUuid const& nodeUuid) const
{
    auto iter = std::find_if(m_targetNodes.begin(), m_targetNodes.end(),
                             [&nodeUuid](TargetNode const& target){
        return target.nodeUuid == nodeUuid;
    });
    if (iter == m_targetNodes.end()) return false;

    return iter->evalType == NodeEvaluationType::KeepEvaluated;
}

bool
GraphExecutionModel::isAutoEvaluatingGraph() const
{
    return m_autoEvaluateGraph;
}

bool
GraphExecutionModel::autoEvaluateGraph()
{
    m_autoEvaluateGraph = true;

    auto const& targetNodes = Impl::findStartAndEndNodes(graph(), PortType::Out);

    for (auto const& target : targetNodes)
    {
        Impl::scheduleTargetNode(*this, target, NodeEvaluationType::KeepEvaluated);
    }

    return true;
}

bool
GraphExecutionModel::autoEvaluateNode(NodeUuid const& nodeUuid)
{
    return Impl::scheduleTargetNode(*this, nodeUuid, NodeEvaluationType::KeepEvaluated) != NodeEvalState::Invalid;
}

FutureNodeEvaluated
GraphExecutionModel::evaluateGraph()
{
    FutureNodeEvaluated future{*this};

    auto const& targetNodes = Impl::findStartAndEndNodes(graph(), PortType::Out);

    for (auto const& target : targetNodes)
    {
        NodeEvalState evalState = Impl::scheduleTargetNode(
            *this, target, NodeEvaluationType::SingleShot
        );

        future.append(target, evalState);
        if (!future.detach()) return future;
    }

    return future;
}

FutureNodeEvaluated
GraphExecutionModel::evaluateNode(NodeUuid const& nodeUuid)
{
    NodeEvalState evalState = Impl::scheduleTargetNode(
        *this, nodeUuid, NodeEvaluationType::SingleShot
    );

    FutureNodeEvaluated future{
        *this, nodeUuid, evalState
    };
    return future;
}

void
GraphExecutionModel::stopAutoEvaluatingGraph()
{
    m_autoEvaluateGraph = false;
    for (auto& target : m_targetNodes)
    {
        target.evalType = NodeEvaluationType::SingleShot;
    }
}

void
GraphExecutionModel::stopAutoEvaluatingNode(const NodeUuid& nodeUuid)
{
    auto iter = std::find_if(m_targetNodes.begin(), m_targetNodes.end(),
                             [nodeUuid](TargetNode const& target){
                                 return target.nodeUuid == nodeUuid;
                             });
    if (iter != m_targetNodes.end()) iter->evalType = NodeEvaluationType::SingleShot;
}

bool
GraphExecutionModel::invalidateNode(NodeUuid const& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item) return false;

    for (auto& port : item->portsIn)
    {
        port.data.state = PortDataState::Outdated;
    }

    return Impl::invalidateNodeHelper(*this, nodeUuid, item);
}

bool
GraphExecutionModel::invalidateNodeOutputs(const NodeUuid& nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item) return false;

    return Impl::invalidateNodeHelper(*this, nodeUuid, item);
}

NodeDataSet
GraphExecutionModel::nodeData(NodeId nodeId, PortId portId) const
{
    return nodeData(graph(), nodeId, portId);
}

NodeDataSet
GraphExecutionModel::nodeData(Graph const& graph, NodeId nodeId, PortId portId) const
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        gtWarning() << getNodeDataError(graph)
                    << tr("node %1 not found!")
                           .arg(nodeId);
        return {};
    }

    return nodeData(node->uuid(), portId);
}

NodeDataSet
GraphExecutionModel::nodeData(NodeUuid const& nodeUuid, PortId portId) const
{
    auto item = Impl::findPortData(*this, nodeUuid, portId, getNodeDataError);
    if (!item) return {};

    return item->data;
}

NodeDataPtrList
GraphExecutionModel::nodeData(NodeUuid const& nodeUuid, PortType type) const
{
    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node) return {};

    auto const& ports = node->ports(type);

    NodeDataPtrList list;
    list.reserve(ports.size());
    for (auto const& port : ports)
    {
        list.push_back({port.id(), nodeData(nodeUuid, port.id())});
    }

    return list;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, NodeDataSet data)
{
    return setNodeData(graph(), nodeId, portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(Graph const& graph, NodeId nodeId, PortId portId, NodeDataSet data)
{
    Node const* node = graph.findNode(nodeId);
    if (!node)
    {
        gtWarning() << setNodeDataError(graph)
                    << tr("node %1 not found!")
                           .arg(nodeId);
        return false;
    }

    return setNodeData(node->uuid(), portId, std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data)
{
    auto item = Impl::findPortData(*this, nodeUuid, portId, setNodeDataError);
    if (!item) return false;

    item->data = std::move(data);

    INTELLI_LOG_SCOPE(*this)
        << tr("setting node data for %1 (%2:%3): port %4, data %5 %6")
               .arg(nodeUuid)
               .arg(item.node->id(), 2)
               .arg(item.node->caption())
               .arg(portId)
               .arg(toString(item->data.ptr)).arg((bool)item->data.state);

    switch (item.portType)
    {
    case PortType::In:
    {
        invalidateNodeOutputs(nodeUuid);

        emit item.node->inputDataRecieved(portId);

        switch (item.entry->nodeType)
        {
        // forward to input provider as outputs
        case NodeType::Group:
        {
            auto* graph = qobject_cast<Graph*>(item.node);
            if(!graph) break; // node may no longer be accessible

            auto* input = graph->inputProvider();
            if(!input) break;

            auto portIdx = graph->portIndex(PortType::In, portId);
            setNodeData(input->uuid(), input->portId(PortType::Out, portIdx), item->data);
            break;
        }
        // forward to parent graph as outputs
        case NodeType::GroupOutput:
        {
            auto* output = qobject_cast<GroupOutputProvider*>(item.node);
            if(!output) break; // node may no longer be accessible

            auto* graph = qobject_cast<Graph*>(output->parent());
            if(!graph) break;

            auto portIdx = output->portIndex(PortType::In, portId);
            setNodeData(graph->uuid(), graph->portId(PortType::Out, portIdx), item->data);
            break;
        }
        // no forwarding required
        case NodeType::GroupInput:
        case NodeType::Normal:
            break;
        }

        // TODO: trigger next nodes when auto evaluating
        break;
    }
    case PortType::Out:
    {
        if (item.requiresReevaluation() || !item.inputsValid())
        {
            item->data.state = PortDataState::Outdated;
            emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());
        }

        // forward data to target nodes
        auto const* graph = qobject_cast<Graph const*>(item.node->parent());

        auto const& connections = graph->findConnections(item.node->id(), portId);

        for (ConnectionId con : connections)
        {
            setNodeData(*graph, con.inNodeId, con.inPort, item->data);
        }
        break;
    }
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }

    return true;
}

bool
GraphExecutionModel::setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data)
{
    auto* node = graph().findNodeByUuid(nodeUuid);
    if (!node)
    {
        gtWarning() << setNodeDataError(this->graph())
                    << tr("node %1 not found!")
                           .arg(nodeUuid);
        return false;
    }

    for (auto& item : data)
    {
        PortId portId = item.first;
        if (!node->port(portId))
        {
            gtWarning() << setNodeDataError(this->graph())
                        << tr("node %1 not found!")
                               .arg(nodeUuid);
            return false;
        }
        if (!setNodeData(nodeUuid, portId, std::move(item.second)))
        {
            return false;

        }
    }

    return true;
}

void
GraphExecutionModel::onNodeEvaluatedHelper()
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node)
    {
        gtError() << graph().objectName() + QStringLiteral(": ")
                  << tr("A node has been evaluated, "
                        "but its object was not found!");
        return emit internalError(QPrivateSignal());
    }

    disconnect(node, &Node::computingFinished,
               this, &GraphExecutionModel::onNodeEvaluatedHelper);

    assert(!Impl::isNodeEvaluating(*node));

    // invoke the actual "onNodeEvaluated" method in the next cycle of
    // Qt's event loop. That way, if a node is executed blockingly, the calling
    // functions can unroll
    QMetaObject::invokeMethod(
        this, "onNodeEvaluatedAsync",
        Qt::QueuedConnection, Q_ARG(QString, node->uuid())
    );
}

void
GraphExecutionModel::onNodeEvaluatedAsync(QString nodeUuid)
{
    auto item = Impl::findData(*this, nodeUuid);
    if (!item)
    {
        gtError() << graph().objectName() + QStringLiteral(": ")
                  << tr("Node %1 has been evaluated, "
                        "but was not found in the model!")
                         .arg(nodeUuid);
        return emit internalError(QPrivateSignal());
    }

    INTELLI_LOG_SCOPE(*this)
        << tr("(ASYNC) node %1 (%2:%3) evaluated!")
               .arg(nodeUuid)
               .arg(item->nodeId, 2)
               .arg(item.node->caption());

    emit nodeEvalStateChanged(nodeUuid, QPrivateSignal());

    if (item.requiresReevaluation())
    {
        INTELLI_LOG(*this)
            << tr("node requires reevaluation!");

        // TODO: check if we need to do some more work here.

        emit item.node->evaluated();

        if (!evaluateNode(nodeUuid).detach())
        {
            INTELLI_LOG(*this)
                << tr("failed to reevaluated!");

            emit nodeEvaluationFailed(nodeUuid, QPrivateSignal());

            emit graphStalled(QPrivateSignal());
        }
        return;
    }

    auto iter = std::find_if(m_targetNodes.begin(), m_targetNodes.end(),
                             [nodeUuid](TargetNode const& target){
        return target.nodeUuid == nodeUuid;
    });
    if (iter != m_targetNodes.end() &&
        iter->evalType == NodeEvaluationType::SingleShot)
    {
        m_targetNodes.erase(iter);
    }

    emit nodeEvaluated(nodeUuid, QPrivateSignal());
    emit item.node->evaluated();

    {
        INTELLI_LOG_SCOPE(*this)
            << tr("rescheduling target nodes...");

        foreach (auto const& targetNode, m_targetNodes)
        {
            Impl::scheduleTargetNode(*this, targetNode.nodeUuid, targetNode.evalType);
        }

        Impl::evaluateNextInQueue(*this);
    }
}

void
GraphExecutionModel::onNodeAppended(Node* node)
{
    auto const appendPorts = [](auto& target, auto const& ports){
        target.reserve(ports.size());

        for (auto& port : ports)
        {
            assert(port.id() != invalid<PortId>());
            target.push_back({port.id()});
        }
    };

    assert(node);

    NodeType type = NodeType::Normal;

    if (auto* g = qobject_cast<Graph*>(node))
    {
        type = NodeType::Group;

        setupConnections(*g);

        auto const& nodes = g->nodes();
        for (auto* n : nodes)
        {
            onNodeAppended(n);
        }
    }
    else if (qobject_cast<GroupInputProvider*>(node))  type = NodeType::GroupInput;
    else if (qobject_cast<GroupOutputProvider*>(node)) type = NodeType::GroupOutput;

    NodeId nodeId = node->id();
    assert(nodeId != invalid<NodeId>());
    NodeUuid const& nodeUuid = node->uuid();
    assert(!nodeUuid.isEmpty());

    if (m_data.contains(nodeUuid))
    {
        INTELLI_LOG(*this)
            << tr("Node %1 already appended!")
                   .arg(nodeUuid);
        return;
    }

    DataItem entry{nodeId};
    appendPorts(entry.portsIn, node->ports(PortType::In));
    appendPorts(entry.portsOut, node->ports(PortType::Out));
    entry.nodeType = type;

    INTELLI_LOG(*this)
        << tr("Node %1 (%2:%3) appended!")
               .arg(nodeUuid)
               .arg(nodeId, 2)
               .arg(node->caption());

    m_data.insert(nodeUuid, std::move(entry));

    disconnect(node);

    connect(node, &Node::triggerNodeEvaluation, this, [this, nodeUuid](){
        invalidateNodeOutputs(nodeUuid);
        if (isAutoEvaluatingGraph() || isAutoEvaluatingNode(nodeUuid))
        {
            Impl::queueNode(*this, nodeUuid, Impl::findData(*this, nodeUuid));
        }
    }, Qt::DirectConnection);

    exec::setNodeDataInterface(*node, *this);
}

void
GraphExecutionModel::onNodeDeleted(Graph* graph, NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());
    assert(graph);

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Node deleted: cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, *graph, nodeId, makeError);
    if (!item) return;

    INTELLI_LOG(*this)
        << tr("Node deleted: updated execution model: %1 (%2:%3)")
               .arg(item.node->uuid())
               .arg(nodeId, 2)
               .arg(item.node->caption());

    m_data.erase(item.entry);

    // TODO: reschedule graph
}

void
GraphExecutionModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert(type != PortType::NoType);
    assert(idx  != invalid<PortIndex>());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Port inserted: cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, nodeId, makeError);
    if (!item) return;

    PortId portId = item.node->portId(type, idx);
    assert(portId != invalid<PortId>());

    INTELLI_LOG(*this)
        << tr("Port inserted: updated execution model: %1 (%2:%3, port %4)")
               .arg(item.node->uuid())
               .arg(nodeId, 2)
               .arg(item.node->caption())
               .arg(portId);

    item.entry->ports(type).push_back({portId});
}

void
GraphExecutionModel::onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert(type != PortType::NoType);
    assert(idx  != invalid<PortIndex>());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Port deleted: cannot update execution model") + ',';
    };

    auto item = Impl::findPortData(*this, nodeId, type, idx, makeError);
    if (!item) return;

    INTELLI_LOG(*this)
        << tr("Port deleted: updated execution model: %1 (%2:%3, port %4)")
               .arg(item.node->uuid())
               .arg(nodeId, 2)
               .arg(item.node->caption())
               .arg(item.portEntry->id);

    item.entry->ports(type).erase(item.portEntry);
}

void
GraphExecutionModel::onConnectedionAppended(Connection* con)
{
    assert(con);
    ConnectionId conId = con->connectionId();
    assert(conId.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection appended: cannot update execution model") + ',';
    };

    auto findOut = Impl::findData(*this, con->outNodeId(), makeError);
    if (!findOut) return;

    auto findIn = Impl::findData(*this, con->inNodeId(), makeError);
    if (!findIn) return;

    // set node data
    auto data = nodeData(findOut.node->uuid(), conId.outPort);
    setNodeData(findIn.node->uuid(), conId.inPort, std::move(data));

    // TODO: reschedule graph
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionId conId)
{
    assert(conId.isValid());

    auto const makeError = [](Graph const& graph){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Connection deleted: cannot update execution model") + ',';
    };

    auto item = Impl::findData(*this, conId.inNodeId, makeError);
    if (!item) return;

    // set node data
    setNodeData(item.node->uuid(), conId.inPort, nullptr);

//    autoEvaluateNode(conId.outNodeId);
//    autoEvaluateNode(conId.inNodeId);
}

void
GraphExecutionModel::onBeginGraphModification()
{
    beginModification();
}

void
GraphExecutionModel::onEndGraphModification()
{
    endModification();
}

void
intelli::debug(GraphExecutionModel const& model)
{
    QString text;
    auto const& graph = model.graph();
    text = QStringLiteral("Graph: ") + graph.caption() + QStringLiteral("\n");

    int base_indent = graph.objectPath().count(';');

    auto const& data = model.data();

    QVector<std::pair<int, QString>> entries;

    auto begin = data.keyValueBegin();
    auto end   = data.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& nodeUuid = iter->first;
        auto& entry = iter->second;

        QString caption QStringLiteral("<NULL_NODE>");
        auto* node = graph.findNodeByUuid(nodeUuid);
        int indent = 1;
        if (node)
        {
            indent = node->objectPath().count(';') - base_indent;
            caption = node->caption();
        }

        entries.push_back({indent, QString{}});
        QString& text = entries.back().second;

        text += QString{"  "}.repeated(indent) +
                QStringLiteral("Node %1: %2 (%3):\n")
                    .arg(entry.nodeId, 2)
                    .arg(caption, nodeUuid);
        text += QString{"  "}.repeated(indent + 1) +
                QStringLiteral("STATE: ");

        switch (entry.state)
        {
        case DataState::RequiresReevaluation:
            text += QStringLiteral("RequiresReevaluation\n");
            break;
        case DataState::Evaluated:
            text += QStringLiteral("Evaluated\n");
            break;
        case DataState::FailedEvaluation:
            text += QStringLiteral("FailedEvaluation\n");
            break;
        }

        for (auto* ports : {&entry.portsIn, &entry.portsOut})
        {
            for (auto& port : *ports)
            {
                text +=
                    QString{"  "}.repeated(indent + 1) +
                    QStringLiteral("Port: %1 (%3) - %2 - %4\n")
                        .arg(port.id)
                        .arg(toString(port.data.ptr),
                             toString(node->portType(port.id)))
                        .arg((bool)port.data.state);
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b){
        return a.first < b.first;
    });

    int oldIndent = 1;
    for (auto& entry : entries)
    {
        if (oldIndent != entry.first)
        {
            oldIndent = entry.first;
            text += QStringLiteral("\n");
        }
        text += entry.second;
    }

    gtInfo().nospace() << "Debugging graph exec model...\n\"\n" << text << "\"";
}

FutureNodeEvaluated::FutureNodeEvaluated(GraphExecutionModel& model) :
    m_model(&model)
{}

FutureNodeEvaluated::FutureNodeEvaluated(GraphExecutionModel& model,
                                         NodeUuid nodeUuid,
                                         NodeEvalState evalState) :
    FutureNodeEvaluated(model)
{
    append(std::move(nodeUuid), evalState);
}

bool
FutureNodeEvaluated::wait(std::chrono::milliseconds timeout)
{
    GT_INTELLI_PROFILE();

    if (!m_model) return false;

    if (isFinished()) return true;

    if (containsFailedNodes()) return false;

    // Nodes are still running -> create local event loop

    // connect event loop here to start recieving updates from exec model
    GtEventLoop loop(timeout);

    loop.connectFailed(m_model.data(), &GraphExecutionModel::internalError);

    // react to node evaluation signals
    auto const& updateLoop = [&loop, this](QString nodeUuid){
        updateTargets();
        if (isFinished()) return emit loop.success();
        if (containsFailedNodes()) return emit loop.failed();
    };

    QObject::connect(m_model, &GraphExecutionModel::nodeEvaluated,
                     &loop, updateLoop);
    QObject::connect(m_model, &GraphExecutionModel::nodeEvaluationFailed,
                     &loop, updateLoop);

    updateTargets();

    if (isFinished()) return true;

    if (containsFailedNodes()) return false;

    // Some nodes are still evaluating

    return loop.exec() == GtEventLoop::Success;
}

bool
FutureNodeEvaluated::detach()
{
    return isFinished() && !containsFailedNodes();
}

FutureNodeEvaluated&
FutureNodeEvaluated::join(FutureNodeEvaluated const& other)
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

FutureNodeEvaluated&
FutureNodeEvaluated::append(NodeUuid nodeUuid, NodeEvalState evalState)
{
    switch (evalState)
    {
    case NodeEvalState::Paused:
    case NodeEvalState::Invalid:
        evalState = NodeEvalState::Invalid;
        // fall through allowed
    case NodeEvalState::Outdated:
    case NodeEvalState::Evaluating:
        // node is evaluating or was evaluated
        m_targets.push_back({std::move(nodeUuid), evalState});
        break;
    case NodeEvalState::Valid:
        // only append nodes that are still running or invalid
        gtTrace().verbose()
            << "[FutureNodeEvaluated]"
            << QObject::tr("Node %1 finished!").arg(nodeUuid);
        break;
    }
    return *this;
}

bool
FutureNodeEvaluated::isFinished() const
{
    return m_targets.empty();
}

bool
FutureNodeEvaluated::containsFailedNodes() const
{
    bool anyInvalid = std::any_of(m_targets.begin(), m_targets.end(),
                                  [](TargetNode const& target){
        // should not contain any valid nodes
        assert(target.evalState != NodeEvalState::Valid);

        return target.evalState == NodeEvalState::Invalid;
    });
    return anyInvalid;
}

void
FutureNodeEvaluated::updateTargets()
{
    for (int idx = m_targets.size() - 1; idx >= 0; idx--)
    {
        // remove entry from list...
        auto target = std::move(m_targets.at(idx));
        m_targets.remove(idx);

        // ...to reappend it only if its still running or invalid
        NodeEvalState state = m_model->nodeEvalState(target.uuid);
        append(std::move(target.uuid), state);
    }
}
