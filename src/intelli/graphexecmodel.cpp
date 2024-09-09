/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/graphexecmodel.h"

#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/connection.h"

#include "intelli/private/utils.h"

#include <gt_exceptions.h>
#include <gt_eventloop.h>

#include <gt_logging.h>

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
#define INTELLI_LOG() gtTrace().verbose()
#else
#define INTELLI_LOG() if (false) gtTrace()
#endif

using namespace intelli;

namespace gt
{
namespace log
{

gt::log::Stream& operator<<(gt::log::Stream& s, graph_data::PortEntry const& e)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace() << "[Port: " << e.id << ", State: " << (int)e.data.state << ", DataPtr: " << e.data.ptr << "]";
    }
    return s;
}

} // namespace log

} // namespace gt

//////////////////////////////////////////////////////

bool
intelli::FutureGraphEvaluated::wait(std::chrono::milliseconds timeout)
{
    if (!m_model) return false;

    if (m_model->isEvaluated()) return true;

    GtEventLoop eventLoop{timeout};

    if (timeout == timeout.max()) eventLoop.setTimeout(-1);

    eventLoop.connectSuccess(m_model, &GraphExecutionModel::graphEvaluated);
    eventLoop.connectFailed(m_model, &GraphExecutionModel::internalError);
    eventLoop.connectFailed(m_model, &GraphExecutionModel::graphStalled);

    return eventLoop.exec() == GtEventLoop::Success;
}

bool
intelli::FutureNodeEvaluated::wait(std::chrono::milliseconds timeout)
{
    if (!m_model) return false;

    if (m_model->isNodeEvaluated(m_targetNode)) return true;

    GtEventLoop eventLoop{timeout};

    if (timeout == timeout.max()) eventLoop.setTimeout(-1);

    eventLoop.connectFailed(m_model, &GraphExecutionModel::internalError);
    eventLoop.connectFailed(m_model, &GraphExecutionModel::graphStalled);

    QObject::connect(m_model, &GraphExecutionModel::nodeEvaluated,
                     &eventLoop, [&eventLoop, this](NodeId nodeId){
        if (nodeId == m_targetNode)
        {
            emit eventLoop.success();
        }
    });

    return eventLoop.exec() == GtEventLoop::Success;
}

NodeDataSet
intelli::FutureNodeEvaluated::get(PortId port, std::chrono::milliseconds timeout)
{
    if (!m_model) return {};

    if (port == invalid<PortId>()) return {};

    if (!wait(timeout)) return {};

    return m_model->nodeData(m_targetNode, port);
}

NodeDataSet
intelli::FutureNodeEvaluated::get(PortType type, PortIndex idx, std::chrono::milliseconds timeout)
{
    if (!m_model) return {};

    auto* node = m_model->graph().findNode(m_targetNode);
    if (!node) return {};

    return get(node->portId(type, idx), timeout);
}

//////////////////////////////////////////////////////

struct GraphExecutionModel::Impl
{

static inline QString
graphName(GraphExecutionModel const& model)
{
    return model.graph().objectName() + QStringLiteral(": ");
}

static inline bool
isNodeEvaluating(Node const* node)
{
    return node && node->nodeFlags() & NodeFlag::Evaluating;
}

template <typename T>
static inline auto&
ports(T& entry, PortType type) noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return entry.portsIn;
    case PortType::Out:
        return entry.portsOut;
    case PortType::NoType:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

template<typename E, typename P>
static inline PortType
portType(E& e, P& p)
{
    if (&e.portsIn == &p) return PortType::In;
    if (&e.portsOut == &p) return PortType::Out;

    return PortType::NoType;
}

template<typename T>
struct PortHelper
{
    T* port;
    PortType type;
    PortIndex idx;
    graph_data::Entry* entry;

    operator T*() { return port; }
    operator T const*() const { return port; }

    T* operator->() { return port; }
    T const* operator->() const { return port; }

    operator PortHelper<T const>() const { return {port, type, idx, entry}; }
};

static PortHelper<graph_data::PortEntry>
findPortDataEntry(GraphExecutionModel& model, NodeId nodeId, PortId portId)
{
    auto entry = model.m_data.find(nodeId);
    if (entry == model.m_data.end())
    {
        gtError() << model.graph().objectName() + ':'
                  << tr("Failed to access port data! (Invalid node %1)").arg(nodeId);
        return { nullptr, PortType::NoType, invalid<PortIndex>(), nullptr };
    }

    PortIndex idx(0);
    for (auto* ports : {&entry->portsIn, &entry->portsOut})
    {
        for (auto& port : *ports)
        {
            if (port.id == portId) return { &port, portType(*entry, *ports), idx, &(*entry) };
            idx++;
        }
        idx = PortIndex(0);
    }

    return { nullptr, PortType::NoType, invalid<PortIndex>(), nullptr };
}

static PortHelper<graph_data::PortEntry const>
findPortDataEntry(GraphExecutionModel const& model, NodeId nodeId, PortId portId)
{
    return findPortDataEntry(const_cast<GraphExecutionModel&>(model), nodeId, portId);
}

template <typename GraphModel_t,
          typename Iter_t = decltype(std::declval<GraphModel_t*>()->m_data.find(NodeId(0))),
          typename Node_t = decltype(std::declval<GraphModel_t*>()->graph().findNode(NodeId(0)))>
struct NodeHelper
{
    GraphModel_t* model;
    Iter_t entry;
    Node_t node;

    operator bool() const
    {
        return model && entry != model->m_data.end() && node;
    }

    bool isEvaluating() const
    {
        return isNodeEvaluating(node);
    }

    bool isEvaluated() const
    {
        return !isEvaluating() && entry->state == NodeState::Evaluated;
    }

    bool requiresReevaluation() const
    {
        return entry->state == NodeState::RequiresReevaluation;
    }

    bool readyForEvaluation() const
    {
        return !isEvaluating() && areInputsValid();
    }

    bool areInputsValid() const
    {
        return std::all_of(entry->portsIn.begin(), entry->portsIn.end(),
                           [this](graph_data::PortEntry const& p){
            auto const& cons = model->graph().findConnections(node->id(), p.id);
            auto* port = node->port(p.id);
            return (cons.empty() || p.data.state == PortDataState::Valid) &&
                   port && ((port->optional) || p.data.ptr);
        });
    }

    bool areInputsNull() const
    {
        return std::any_of(entry->portsIn.begin(), entry->portsIn.end(),
                           [](graph_data::PortEntry const& p){
            return !p.data.ptr;
        });
    }

    bool areOutputsValid() const
    {
        return std::all_of(entry->portsOut.begin(), entry->portsOut.end(),
                           [](graph_data::PortEntry const& p){
            return p.data.state == PortDataState::Valid;
        });
    }

    bool areOutputsNull() const
    {
        return std::any_of(entry->portsOut.begin(), entry->portsOut.end(),
                           [](graph_data::PortEntry const& p){
                               return !p.data.ptr;
                           });
    }
};

template <typename T>
static inline NodeHelper<T>
findNode(T& model, NodeId nodeId)
{
    return {
        &model,
        model.m_data.find(nodeId),
        model.graph().findNode(nodeId)
    };
}

static inline QVector<NodeId>
findStartAndEndNodes(Graph const& graph, QList<Node*> const& nodes, PortType type)
{
    QVector<NodeId> targetNodes;
    for (auto const* node : nodes)
    {
        if (graph.findConnections(node->id(), type).empty())
        {
            targetNodes.push_back(node->id());
        }
    }
    return targetNodes;
}

static inline bool
doTriggerNode(GraphExecutionModel& model, Node* node)
{
    assert(node);
    auto entry = model.m_data.find(node->id());
    assert (entry != model.m_data.end());

    // subscribe to when node finished its
    QObject::connect(node, &Node::computingFinished,
                     &model, &GraphExecutionModel::onNodeEvaluated,
                     Qt::UniqueConnection);

    auto cleanup = gt::finally([&model, node](){
        emit model.nodeEvalStateChanged(node->id());
        QObject::disconnect(node, &Node::computingFinished,
                            &model, &GraphExecutionModel::onNodeEvaluated);
    });

    entry->state = NodeState::Evaluated;

    gtInfo().medium().nospace()
        << Impl::graphName(model)
        << tr("Evaluating node %1...").arg(node->id());

    // evaluate nodes
    if (!node->handleNodeEvaluation(model))
    {
        return false;
    }

    cleanup.clear();

    return true;
}

}; // struct Impl;

GraphExecutionModel::GraphExecutionModel(Graph& graph, Mode mode) :
    m_mode(mode)
{
    if (graph.executionModel())
    {
        gtError() << tr("Graph '%1' already has an execution model!")
                         .arg(graph.objectName());
    }

    setObjectName(QStringLiteral("__exec_model"));
    setParent(&graph);

    m_graph = &graph;

    reset();

    connect(this, &GraphExecutionModel::nodeEvaluated, this, [this](NodeId nodeId){
        gtInfo().nospace().medium()
            << Impl::graphName(*this)
            << tr("Node %1 evaluated!").arg(nodeId);
    });
    connect(this, &GraphExecutionModel::graphEvaluated, this, [this, &graph](){
        gtInfo().nospace().medium()
            << Impl::graphName(*this)
            << tr("Graph '%1' evaluated!").arg(graph.objectName());
    });
    connect(this, &GraphExecutionModel::graphStalled, this, [this, &graph](){
        gtInfo().nospace().medium()
            << Impl::graphName(*this)
            << tr("Graph '%1' stalled!").arg(graph.objectName());
    });

    connect(&graph, &Graph::nodeAppended,
            this, &GraphExecutionModel::onNodeAppended, Qt::DirectConnection);
    connect(&graph, &Graph::childNodeAboutToBeDeleted,
            this, &GraphExecutionModel::onNodeDeleted, Qt::DirectConnection);
    connect(&graph, &Graph::connectionAppended,
            this, &GraphExecutionModel::onConnectedionAppended, Qt::DirectConnection);
    connect(&graph, &Graph::connectionDeleted,
            this, &GraphExecutionModel::onConnectionDeleted, Qt::DirectConnection);
    connect(&graph, &Graph::nodePortInserted,
            this, &GraphExecutionModel::onNodePortInserted, Qt::DirectConnection);
    connect(&graph, &Graph::nodePortAboutToBeDeleted,
            this, &GraphExecutionModel::onNodePortAboutToBeDeleted, Qt::DirectConnection);
    connect(&graph, &Graph::beginModification,
            this, &GraphExecutionModel::onBeginGraphModification, Qt::DirectConnection);
    connect(&graph, &Graph::endModification,
            this, &GraphExecutionModel::onEndGraphModification, Qt::DirectConnection);
}

GraphExecutionModel::~GraphExecutionModel() = default;

Graph&
GraphExecutionModel::graph()
{
    assert(m_graph);
    return *m_graph;
}

const Graph&
GraphExecutionModel::graph() const
{
    return const_cast<GraphExecutionModel*>(this)->graph();
}

void
GraphExecutionModel::makeActive()
{
    m_mode = ActiveModel;
}

GraphExecutionModel::Mode
GraphExecutionModel::mode() const
{
    return m_mode;
}

void
GraphExecutionModel::beginModification()
{
    gtTrace().verbose().nospace()
        << Impl::graphName(*this) << tr("BEGIN MODIFICIATION...")
        << m_modificationCount;

    assert(m_modificationCount >= 0);
    m_modificationCount++;
}

void
GraphExecutionModel::endModification()
{
    m_modificationCount--;
    assert(m_modificationCount >= 0);

    gtTrace().verbose().nospace()
        << Impl::graphName(*this) << tr("...END MODIFICATION")
        << m_modificationCount;

    if (m_modificationCount == 0) evaluateNextInQueue();
}

void
GraphExecutionModel::reset()
{
    beginReset();
    endReset();
}

void
GraphExecutionModel::beginReset()
{
    m_autoEvaluate = false;

    auto& graph = this->graph();
    auto iter = m_data.keyBegin();
    auto end  = m_data.keyEnd();
    for (; iter != end; ++iter)
    {
        auto& entry = *m_data.find(*iter);
        auto* node = graph.findNode(*iter);
        assert(node);
        entry.state = NodeState::RequiresReevaluation;
        for (auto& entry : entry.portsIn ) entry.data.state = PortDataState::Outdated;
        for (auto& entry : entry.portsOut) entry.data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_data.clear();
    m_targetNodes.clear();
    m_pendingNodes.clear();
    m_queuedNodes.clear();
    m_evaluatingNodes.clear();

    auto const& nodes = graph().nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }

    // reset subgraphs
    auto const& subgraphs = graph().graphNodes();
    for (auto *subgraph : subgraphs)
    {
        assert(subgraph);
        if (auto* exec = subgraph->executionModel())
        {
            exec->reset();
        }
    }
}

NodeEvalState
GraphExecutionModel::nodeEvalState(NodeId nodeId) const
{
    auto find = Impl::findNode(*this, nodeId);
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

        if (!find.areOutputsValid() || find.areOutputsNull())
        {
            return NodeEvalState::Invalid;
        }

        if (find.node->ports(PortType::Out).empty() && find.areInputsNull())
        {
            return NodeEvalState::Invalid;
        }

        return NodeEvalState::Valid;
    }

    return NodeEvalState::Outdated;
}

bool
GraphExecutionModel::isEvaluated() const
{
    return std::all_of(m_data.keyBegin(), m_data.keyEnd(), [this](NodeId nodeId){
        return isNodeEvaluated(nodeId);
    });
}

bool
GraphExecutionModel::isNodeEvaluated(NodeId nodeId) const
{
    auto find = Impl::findNode(*this, nodeId);

    return find && find.isEvaluated();
}

bool
GraphExecutionModel::isAutoEvaluating() const
{
    return m_autoEvaluate;
}

void
GraphExecutionModel::disableAutoEvaluation()
{
    m_autoEvaluate = false;
}

FutureGraphEvaluated
GraphExecutionModel::autoEvaluate()
{
    m_autoEvaluate = true;

    auto& graph = this->graph();

    auto const& cyclicNodes = intelli::cyclicNodes(graph);
    if (!cyclicNodes.empty())
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("Cannot auto evaluate cyclic graph! The node sequence %1 "
                  "contains a cycle!").arg(toString(cyclicNodes));
        m_autoEvaluate = false;
        return {};
    }

    auto const& nodes = graph.nodes();
    auto const& rootNodes = findRootNodes(nodes);

    if (!nodes.empty() && rootNodes.empty())
    {
        gtError().nospace()
            << Impl::graphName(*this).chopped(1)
            << tr("Failed to find root nodes to begin graph evaluation!");
        m_autoEvaluate = false;
        return {};
    }

    bool success = false;
    for (NodeId nodeId : qAsConst(rootNodes))
    {
        success |= autoEvaluateNode(nodeId);
    }

    return FutureGraphEvaluated{success ? this : nullptr};
}

bool
GraphExecutionModel::autoEvaluateNode(NodeId nodeId)
{
    if (!m_autoEvaluate) return false;

    auto& graph = this->graph();

    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Failed to auto evaluate node %1!").arg(nodeId);
    };

    // get node
    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << makeError() << tr("(Node not found)");
        return false;
    }

    // cannot auto evaluate inactive nodes
    if (!find.node->isActive())
    {
        INTELLI_LOG()
            << makeError()
            << tr("(node %1 is not active)").arg(nodeId);
        return false;
    }

    // node is already evaluating
    if (find.isEvaluating())
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is already evaluating)");
        // nothing to do
        return true;
    }

    // queue this node
    if (!find.isEvaluated())
    {
        return queueNodeForEvaluation(nodeId);
    }

    // evaluate next nodes
    INTELLI_LOG()
        << makeError()
        << tr("(Node was already evaluated)");

    auto const& targetNodes = graph.findConnectedNodes(nodeId, PortType::Out);

    bool success = targetNodes.empty();

    for (NodeId nextNode : targetNodes) success |= autoEvaluateNode(nextNode);

    return success;
}

FutureGraphEvaluated
GraphExecutionModel::evaluateGraph()
{
    auto& graph = this->graph();
    auto const& nodes = graph.nodes();

    // no nodes -> graph already evaluated
    if (nodes.empty())
    {
        return FutureGraphEvaluated(this);
    }

    // find target nodes (nodes with no output connections)
    auto const& leafNodes = findLeafNodes(nodes);
    if (leafNodes.empty())
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("Failed to evaluate graph, target nodes not found!");
        return {};
    }

    // evaluate until all leaf nodes
    for (NodeId target : qAsConst(leafNodes))
    {
        if (!evaluateNode(target).detach())
        {
            return {};
        }
    }

    return FutureGraphEvaluated(this);
}

FutureNodeEvaluated
GraphExecutionModel::evaluateNode(NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());

    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Failed to evaluate target node %1!").arg(nodeId);
    };

    if (m_targetNodes.contains(nodeId))
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is already marked for evaluation)");
        return FutureNodeEvaluated(this, nodeId);
    }

    m_targetNodes.push_back(nodeId);

    if (!evaluateNodeDependencies(nodeId))
    {
        // restore target and pending lists
        m_targetNodes.removeOne(nodeId);
        rescheduleTargetNodes();
        return {};
    }

    return FutureNodeEvaluated(this, nodeId);
}

bool
GraphExecutionModel::evaluateNodeDependencies(NodeId nodeId, bool reevaluate)
{
    assert(nodeId != invalid<NodeId>());

    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Failed to schedule node %1!").arg(nodeId);
    };

    // node is already pending
    if (!reevaluate && m_pendingNodes.contains(nodeId))
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is already pending)");
        return true;
    }

    // get node
    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << makeError() << tr("(Node not found)");
        return false;
    }

    // node is already evaluating
    if (find.isEvaluating())
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is already evaluating)");
        return true;
    }

    // node was already evaluated
    if (find.isEvaluated())
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node was already evaluated)");
        return true;
    }

    // add node to pending nodes
    if (!reevaluate) m_pendingNodes.push_back(nodeId);

    // try queueing
    if (queueNodeForEvaluation(nodeId))
    {
        return true;
    }

    // node not ready to be queued -> dependencies not fullfilled
    auto const& dependencies = graph().findConnectedNodes(nodeId, PortType::In);

    INTELLI_LOG().nospace()
        << Impl::graphName(*this)
        << tr("Node %1 is not ready for evaluation. Checking dependencies: %2")
               .arg(nodeId).arg(toString(dependencies));

    if (dependencies.empty())
    {
        gtError() << makeError() << tr("(Node is not ready and has no dependencies)");
        return false;
    }

    for (NodeId dependency : dependencies)
    {
        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("Checking dependency: ") << dependency;

        assert (dependency != nodeId);
        if (!evaluateNodeDependencies(dependency)) return false;
    }

    return true;
}

bool
GraphExecutionModel::queueNodeForEvaluation(NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());

    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Failed to queue node %1!").arg(nodeId);
    };

    // check if node is already queued
    if (m_queuedNodes.contains(nodeId))
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is already queued)");
        return true;
    }

    // get node
    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node not found)");
        return false;
    }

    // is node ready for evaluation?
    if (!find.readyForEvaluation())
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Node is not ready for evaluation, "
                  "some inputs are not valid yet)");
        return false;
    }

    // schedule node
    INTELLI_LOG().nospace()
        << Impl::graphName(*this)
        << tr("Queuing node %1...").arg(nodeId);

    m_queuedNodes.push_back(nodeId);
    m_pendingNodes.removeOne(nodeId);

    evaluateNextInQueue();

    return true;
}

bool
GraphExecutionModel::evaluateNextInQueue()
{
    auto const makeError = [this](Node* node = nullptr){
        return Impl::graphName(*this) +
               tr("Failed to schedule evaluation of node %1!").arg(node ? node->id() : -1);
    };

    // do not evaluate if graph is currently being modified
    if (m_modificationCount > 0)
    {
        INTELLI_LOG()
            << makeError()
            << tr("(Model is inserting)");
        return false;
    }

    bool scheduledNode = false;

    // for each node in queue
    for (int idx = 0; idx <= m_queuedNodes.size() - 1; ++idx)
    {
        NodeId nodeId = m_queuedNodes.at(idx);

        // get node
        auto* node = graph().findNode(nodeId);
        if (!node)
        {
            m_queuedNodes.removeAt(idx--);
            continue;
        }

        assert(!m_evaluatingNodes.contains(node));
        assert(!Impl::isNodeEvaluating(node));

        // check if we are currently evaluating exclusive nodes
        auto containsExclusiveNodes =
            std::any_of(m_evaluatingNodes.cbegin(), m_evaluatingNodes.cend(),
                        [](QPointer<Node> const& evaluating){
            return evaluating && evaluating->nodeEvalMode() == NodeEvalMode::Exclusive;
        });

        // an exclusive node has to be evaluated separatly to all other nodes
        if (containsExclusiveNodes)
        {
            INTELLI_LOG()
                << makeError()
                << tr("(Executor contains exclusive nodes)");
            return false;
        }

        if (node->nodeEvalMode() == NodeEvalMode::Exclusive && !m_evaluatingNodes.empty())
        {
            INTELLI_LOG()
                << makeError()
                << tr("(Node is exclusive and executor is not empty)");
            continue;
        }

        m_evaluatingNodes.push_back(node);
        m_queuedNodes.removeAt(idx--);

        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("Evaluating node %1...").arg(nodeId);

        // trigger execution
        if (!Impl::doTriggerNode(*this, node))
        {
            gtError() << makeError(node) << tr("(Node execution failed)");
            // node may already be removed here
            m_evaluatingNodes.removeOne(node);
            continue;
        }

        // success
        scheduledNode = true;
    }

    return scheduledNode;
}

void
GraphExecutionModel::rescheduleTargetNodes()
{
    m_pendingNodes.clear();
    for (int idx = m_targetNodes.size() - 1; idx >= 0; --idx)
    {
        if (!evaluateNodeDependencies(m_targetNodes.at(idx)))
        {
            m_targetNodes.removeAt(idx);
        }
    }
}

QVector<NodeId>
GraphExecutionModel::findRootNodes(QList<Node*> const& nodes) const
{
    return Impl::findStartAndEndNodes(this->graph(), nodes, PortType::In);
}

QVector<NodeId>
GraphExecutionModel::findLeafNodes(QList<Node*> const& nodes) const
{
    return Impl::findStartAndEndNodes(this->graph(), nodes, PortType::Out);
}

void
GraphExecutionModel::debug(NodeId nodeId) const
{
    if (nodeId != invalid<NodeId>())
    {
        auto find = Impl::findNode(*this, nodeId);
        if (!find) return;

        gtDebug() << Impl::graphName(*this) << "#######################";
        gtDebug() << Impl::graphName(*this) << "Node:     " << nodeId << find.node->caption() << "| State:" << (int)find.entry->state;
        gtDebug() << Impl::graphName(*this) << "In-Ports: " << find.entry->portsIn;
        gtDebug() << Impl::graphName(*this) << "Out-Ports:" << find.entry->portsOut;
        gtDebug() << Impl::graphName(*this) << "#######################";
        return;
    }

    QVector<NodeId> evaluating;
    std::transform(m_evaluatingNodes.begin(), m_evaluatingNodes.end(), std::back_inserter(evaluating),
                   [](QPointer<Node> const& node){ return node->id(); });

    gtDebug() << Impl::graphName(*this) << "#######################";
    gtDebug() << Impl::graphName(*this) << "target nodes:    " << m_targetNodes;
    gtDebug() << Impl::graphName(*this) << "pending nodes:   " << m_pendingNodes;
    gtDebug() << Impl::graphName(*this) << "queued nodes:    " << m_queuedNodes;
    gtDebug() << Impl::graphName(*this) << "evaluating nodes:" << evaluating;
    gtDebug() << Impl::graphName(*this) << "#######################";
}

bool
GraphExecutionModel::invalidatePortEntry(NodeId nodeId, graph_data::PortEntry& port)
{
    if (port.data.state != PortDataState::Valid) return true; // nothing to do here

    auto find = Impl::findNode(*this, nodeId);
    if (!find) return false;

    port.data.state = PortDataState::Outdated;

    INTELLI_LOG().nospace()
        << Impl::graphName(*this)
        << "Invalidating node " << nodeId << " port id " << port.id;

    PortType type = find.node->portType(port.id);

    bool success = true;
    switch (type)
    {
    case PortType::Out:
    {
        find.entry->state = NodeState::RequiresReevaluation;

        for (auto& con : graph().findConnections(nodeId, PortType::Out))
        {
            success &= invalidatePort(con.inNodeId, con.inPort);
        }
        break;
    }
    case PortType::In:
        emit nodeEvalStateChanged(nodeId);
        success = invalidateOutPorts(nodeId);
        break;
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }

    emit nodeEvalStateChanged(nodeId);
    return success;
}

bool
GraphExecutionModel::invalidateOutPorts(NodeId nodeId)
{
    auto find = Impl::findNode(*this, nodeId);
    if (!find) return false;

    find.entry->state = NodeState::RequiresReevaluation;
    emit nodeEvalStateChanged(nodeId);

    bool success = true;
    for (auto& port : find.entry->portsOut)
    {
        success &= invalidatePortEntry(nodeId, port);
    }
    return success;
}

bool
GraphExecutionModel::invalidatePort(NodeId nodeId, PortId portId)
{
    auto port = Impl::findPortDataEntry(*this, nodeId, portId);
    if (!port) return false;
    
    invalidatePortEntry(nodeId, *port);

    return true;
}

bool
GraphExecutionModel::invalidateNode(NodeId nodeId)
{
    return invalidateOutPorts(nodeId);
}

NodeDataSet
GraphExecutionModel::nodeData(NodeId nodeId, PortId portId) const
{
    auto port = Impl::findPortDataEntry(*this, nodeId, portId);
    if (!port)
    {
        gtWarning().nospace()
            << Impl::graphName(*this)
            << tr("Accessing data of node %1 failed! (Port id %2 not found)")
                    .arg(nodeId).arg(portId);
        return {};
    }

    return port->data;
}

NodeDataSet
GraphExecutionModel::nodeData(NodeId nodeId, PortType type, PortIndex idx) const
{
    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtWarning().nospace()
            << Impl::graphName(*this)
            << tr("Accessing data of node %1 failed! (Node not found)")
                   .arg(nodeId);
        return {};
    }

    return nodeData(nodeId, node->portId(type, idx));
}

NodeDataPtrList
GraphExecutionModel::nodeData(NodeId nodeId, PortType type) const
{
    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("Failed to access node data! (Node %1 not found)").arg(nodeId);
        return {};
    }

    NodeDataPtrList data;

    auto& ports = Impl::ports(*find.entry, type);
    for (auto& port : ports)
    {
        data.push_back({find.node->portIndex(type, port.id), port.data});
    }
    return data;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, NodeDataSet data)
{
    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Setting data for node %1 failed!").arg(nodeId);
    };

    auto& graph = this->graph();

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtWarning() << makeError() << tr("(Node not found)");
        return false;
    }

    auto tmp = Impl::findPortDataEntry(*this, nodeId, portId);
    auto port = tmp.port;
    auto type = tmp.type;
    if (!port)
    {
        gtWarning() << makeError() << tr("(Port %1 not found)").arg(portId);
        return false;
    }

    // data is same, nothing to do
    if (port->data.ptr == data.ptr &&
        port->data.state == data.state &&
        data.state == PortDataState::Valid)
    {
        INTELLI_LOG()
            << Impl::graphName(*this)
            << tr("(Not setting port data for node %1:%2, "
                  "data did not change)")
                   .arg(nodeId).arg(portId);
        return true;
    }

    port->data = std::move(data);

    switch (type)
    {
    case PortType::In:
    {
        invalidateOutPorts(nodeId);

        emit find.node->inputDataRecieved(portId);

        if (m_autoEvaluate)
        {
            // check if source node is still evaluating -> else trigger node evaluation
            auto const& dependencies = graph.findConnectedNodes(nodeId, portId);

            bool isEvaluating = std::any_of(dependencies.begin(),
                                            dependencies.end(),
                                            [&graph](NodeId dependency){
                return Impl::isNodeEvaluating(graph.findNode(dependency));
            });

            if (!isEvaluating)
            {
                INTELLI_LOG()
                    << Impl::graphName(*this)
                    << tr("Triggering node %1 from input data").arg(nodeId);
                autoEvaluateNode(nodeId);
            }
        }
        break;
    }
    case PortType::Out:
    {
        if (!find.areInputsValid() || find.requiresReevaluation())
        {
            port->data.state = PortDataState::Outdated;
        }

        // forward data to target nodes
        auto const& connections = graph.findConnections(nodeId, portId);

        for (ConnectionId con : connections)
        {
            setNodeData(con.inNodeId, con.inPort, port->data);
        }
        break;
    }
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }

    emit nodeEvalStateChanged(nodeId);

    return true;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeDataSet data)
{
    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtWarning().nospace()
            << Impl::graphName(*this)
            << tr("Setting data of node %1 failed! (Node not found)")
                   .arg(nodeId);
        return {};
    }

    return setNodeData(nodeId, node->portId(type, idx), std::move(data));
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId,
                                 PortType type,
                                 NodeDataPtrList const& data)
{
    PortIndex idx(0);
    for (auto d : data)
    {
        if (!setNodeData(nodeId, type, d.first, std::move(d.second)))
        {
            return false;
        }
        idx++;
    }

    return true;
}

void
GraphExecutionModel::onNodeAppended(Node* node)
{
    assert(node);

    graph_data::Entry entry{};

    auto const& inPorts  = node->ports(PortType::In);
    auto const& outPorts = node->ports(PortType::Out);

    entry.portsIn.reserve(inPorts.size());
    entry.portsOut.reserve(outPorts.size());

    for (auto& port : inPorts)
    {
        entry.portsIn.push_back({port.id()});
    }

    for (auto& port : outPorts)
    {
        entry.portsOut.push_back({port.id()});
    }

    m_data.insert(node->id(), std::move(entry));

    /// evaluate node if triggered
    connect(node, &Node::triggerNodeEvaluation,
            this, [this, nodeId = node->id()](){
        invalidateOutPorts(nodeId);
        autoEvaluateNode(nodeId);
    });

    connect(node, &Node::isActiveChanged,
            this, std::bind(&GraphExecutionModel::nodeEvalStateChanged, this, node->id()));
    connect(node, &Node::computingStarted,
            this, std::bind(&GraphExecutionModel::nodeEvalStateChanged, this, node->id()));
    connect(node, &Node::evaluated,
            this, std::bind(&GraphExecutionModel::nodeEvalStateChanged, this, node->id()));
}
void
GraphExecutionModel::onNodeDeleted(NodeId nodeId)
{
    m_data.remove(nodeId);

    bool evaluationQueueChanged = false;
    evaluationQueueChanged |= m_evaluatingNodes.removeOne(nullptr);
    evaluationQueueChanged |= m_queuedNodes.removeOne(nodeId);

    bool pendingNodeRemoved = false;
    pendingNodeRemoved |= m_pendingNodes.removeOne(nodeId);
    pendingNodeRemoved |= m_targetNodes.removeOne(nodeId);

    if (pendingNodeRemoved)
    {
        rescheduleTargetNodes();
    }
    else if (evaluationQueueChanged)
    {
        evaluateNextInQueue();
    }
}

void
GraphExecutionModel::onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert (type != PortType::NoType);

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << tr("Failed to insert port index %1 (%2) for node %3! "
                        "(node entry not found)")
                         .arg(idx).arg(toString(type)).arg(nodeId);
        return;
    }

    auto portId = find.node->portId(type, idx);
    assert(portId != invalid<PortId>());

    auto& entries = type == PortType::In ?
                        find.entry->portsIn :
                        find.entry->portsOut;
    entries.push_back({portId});
}

void
GraphExecutionModel::onNodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx)
{
    assert (type != PortType::NoType);

    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtError() << tr("Failed to delete port index %1 (%2) from node %3! "
                        "(node object not found)")
                         .arg(idx).arg(toString(type)).arg(nodeId);
        return;
    }

    auto find = Impl::findPortDataEntry(*this, nodeId, node->portId(type, idx));
    if (!find)
    {
        gtError() << tr("Failed to delete port index %1 (%2) from node %3! "
                        "(node port not found)")
                         .arg(idx).arg(toString(type)).arg(nodeId);
        return;
    }

    auto& entries = type == PortType::In ?
                        find.entry->portsIn :
                        find.entry->portsOut;
    entries.removeAt(find.idx);
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
GraphExecutionModel::onConnectedionAppended(Connection* con)
{
    assert(con);
    ConnectionId conId = con->connectionId();

    auto const makeError = [this, conId](){
        return graph().objectName() + ": " +
               tr("Failed to integrate new connection %1!").arg(toString(conId));
    };

    auto find = Impl::findNode(*this, con->outNodeId());
    if (!find)
    {
        gtError() << makeError() << tr("(out node not found)");
        return;
    }

    // set node data
    auto data = nodeData(conId.outNodeId, conId.outPort);
    setNodeData(conId.inNodeId, conId.inPort, std::move(data));

    // try to auto evaluate node else forward node data
    if (!isNodeEvaluated(conId.outNodeId)) autoEvaluateNode(conId.outNodeId);

    // to be safe
    rescheduleTargetNodes();
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionId conId)
{
    setNodeData(conId.inNodeId, conId.inPort, nullptr);

    autoEvaluateNode(conId.outNodeId);
    autoEvaluateNode(conId.inNodeId);

    // to be safe
    rescheduleTargetNodes();
}

void
GraphExecutionModel::onNodeEvaluated()
{
    auto const& graph = this->graph();

    auto* node = qobject_cast<Node*>(sender());
    if (!node)
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("A node has evaluated, but its object was not found!");
        return emit internalError();
    }

    disconnect(node, &Node::computingFinished,
               this, &GraphExecutionModel::onNodeEvaluated);

    NodeId nodeId = node->id();

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("Node %1 has evaluated, but was no "
                  "longer found in the execution model!").arg(nodeId);
        return emit internalError();
    }

    assert(node == find.node);

    if (find.isEvaluating())
    {
        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("Node %1 is still evaluating!").arg(nodeId);
        return;
    }

    m_evaluatingNodes.removeOne(node);

    if (find.requiresReevaluation())
    {
        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("Node %1 requieres reevaluation!").arg(nodeId);

        emit nodeEvaluated(nodeId);
        if (!autoEvaluateNode(nodeId))
        {
            emit graphStalled();
        }
        return;
    }

    // node does not require evaluation -> assume all outports are valid
//    if (find.areInputsValid())
//    {
//        for (auto& port : find.entry->portsOut)
//        {
//            if (port.state == PortDataState::Outdated)
//            {
//                setNodeData(nodeId, port.id, port.data);
//            }
//        }
//    }

    m_targetNodes.removeOne(nodeId);

    emit nodeEvaluated(nodeId);

    // queue next nodes
    if (m_autoEvaluate)
    {
        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("Triggering next nodes...");

        auto const& nextNodes = graph.findConnectedNodes(nodeId, PortType::Out);

        for (NodeId nextNode : nextNodes)
        {
            autoEvaluateNode(nextNode);
        }

        INTELLI_LOG().nospace()
            << Impl::graphName(*this)
            << tr("...done!");
    }

    // nodes may evaluate blockingly, and mess with pending nodes list
    // -> prevent modifications
    if (!m_evaluatingPendingNodes)
    {
        m_evaluatingPendingNodes = true;
        auto finally = gt::finally([this](){
            m_evaluatingPendingNodes = false;
        });

        // dependencies of pending nodes may have been resolved
        // -> attempt to reschedule pending nodes
        for (int idx = m_pendingNodes.size() - 1; idx >= 0; --idx)
        {
            NodeId pending = m_pendingNodes.at(idx);
            evaluateNodeDependencies(pending, true);
        }

        // trigger evaluation
        if (!evaluateNextInQueue() &&
            m_evaluatingNodes.empty() &&
            m_queuedNodes.empty() &&
            !m_pendingNodes.empty())
        {
            m_targetNodes.clear();
            m_pendingNodes.clear();
            emit graphStalled();
        }
    }

    if (isEvaluated()) emit graphEvaluated();
}
