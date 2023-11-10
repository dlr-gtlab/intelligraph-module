/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/graphexecmodel.h"

#include "intelli/nodeexecutor.h"
#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/connection.h"

#include "intelli/private/utils.h"
#include "intelli/private/node_impl.h"

#include <gt_exceptions.h>
#include <gt_eventloop.h>

#include <gt_logging.h>

using namespace intelli;

//////////////////////////////////////////////////////

DummyDataModel::DummyDataModel(Node& node) :
    m_node(&node)
{
    NodeExecutor::setNodeDataInterface(node, this);

    auto const& inPorts  = node.ports(PortType::In);
    auto const& outPorts = node.ports(PortType::Out);

    m_data.portsIn.reserve(inPorts.size());
    m_data.portsOut.reserve(outPorts.size());

    for (auto& port : inPorts)
    {
        m_data.portsIn.push_back({port.id()});
    }

    for (auto& port : outPorts)
    {
        m_data.portsOut.push_back({port.id()});
    }
}

dm::NodeDataSet
DummyDataModel::nodeData(NodeId nodeId, PortId portId) const
{
    assert(m_node);
    if (nodeId != m_node->id())
    {
        gtError() << QObject::tr("Failed to access node %1! (Was expecting node %2)")
                         .arg(nodeId).arg(nodeId);
        return {};
    }

    for (auto const* ports : {&m_data.portsIn, &m_data.portsOut})
    {
        for (auto const& p : *ports)
        {
            if (p.id == portId) return p.data;
        }
    }

    gtError() << QObject::tr("Failed to access data of node %1! (Port id %2 not found)")
                     .arg(nodeId).arg(portId);
    return {};
}

dm::NodeDataSet
DummyDataModel::nodeData(PortId portId, dm::NodeDataSet data)
{
    assert(m_node);
    return nodeData(m_node->id(), portId);
}

NodeDataPtrList
DummyDataModel::nodeData(PortType type) const
{
    assert(m_node);
    NodeDataPtrList data;

    auto const& ports = type == PortType::In ? &m_data.portsIn : &m_data.portsOut;
    for (auto& port : *ports)
    {
        data.push_back({m_node->portIndex(type, port.id), port.data});
    }
    return data;
}

bool
DummyDataModel::setNodeData(NodeId nodeId, PortId portId, dm::NodeDataSet data)
{
    assert(m_node);
    if (nodeId != m_node->id())
    {
        gtError() << QObject::tr("Failed to access node %1! (Was expecting node %2)")
                         .arg(nodeId).arg(nodeId);
        return false;
    }

    for (auto* ports : {&m_data.portsIn, &m_data.portsOut})
    {
        for (auto& p : *ports)
        {
            if (p.id == portId)
            {
                p.data = std::move(data.data);
                return true;
            }
        }
    }

    gtError() << QObject::tr("Failed to set data of node %1! (Port id %2 not found)")
                     .arg(nodeId).arg(portId);
    return {};
}

bool
DummyDataModel::setNodeData(PortId portId, dm::NodeDataSet data)
{
    assert(m_node);
    return setNodeData(m_node->id(), portId, std::move(data));
}

bool
DummyDataModel::setNodeData(PortType type, const NodeDataPtrList& data)
{
    assert(m_node);
    for (auto& d : data)
    {
        if (!setNodeData(m_node->portId(type, d.first), std::move(d.second)))
        {
            return false;
        }
    }
    return true;
}

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

dm::NodeDataSet
intelli::FutureNodeEvaluated::get(PortId port, std::chrono::milliseconds timeout)
{
    if (!m_model) return {};

    if (port == invalid<PortId>()) return {};

    if (!wait(timeout)) return {};

    return m_model->nodeData(m_targetNode, port);
}

dm::NodeDataSet
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
    dm::Entry* entry;

    operator T*() { return port; }
    operator T const*() const { return port; }

    operator PortHelper<T const>() const { return {port, type, idx, entry}; }
};

static PortHelper<dm::PortEntry>
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

static PortHelper<dm::PortEntry const>
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

    bool isEvaluated() const
    {
        return entry->isEvaluated(*node);
    }

    bool canEvaluate() const
    {
        return entry->canEvaluate(model->graph(), *node);
    }

    bool areInputsValid() const
    {
        return entry->areInputsValid(model->graph(), node->id());
    }
};

template <typename T>
inline static NodeHelper<T>
findNode(T& model, NodeId nodeId)
{
    return {
        &model,
        model.m_data.find(nodeId),
        model.graph().findNode(nodeId)
    };
}

inline static bool
doTriggerNode(GraphExecutionModel& model, Node* node)
{
    // subscribe to when node finished its
    QObject::connect(node, &Node::computingFinished,
                     &model, &GraphExecutionModel::onNodeEvaluated,
                     Qt::UniqueConnection);

    auto cleanup = gt::finally([&model, node](){
        QObject::disconnect(node, &Node::computingFinished,
                            &model, &GraphExecutionModel::onNodeEvaluated);
    });

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

    setObjectName("__exec_model");
    setParent(&graph);

    m_graph = &graph;

    reset();

    connect(this, &GraphExecutionModel::nodeEvaluated, this, [this](NodeId nodeId){
        gtDebug().nospace()
            << Impl::graphName(*this)
            << tr("Node %1 evaluated!").arg(nodeId);
    });
    connect(this, &GraphExecutionModel::graphEvaluated, this, [this, &graph](){
        gtDebug().nospace()
            << Impl::graphName(*this)
            << tr("Graph '%1' evaluated!").arg(graph.objectName());
    });
    connect(this, &GraphExecutionModel::graphStalled, this, [this, &graph](){
        gtWarning().nospace()
            << Impl::graphName(*this)
            << tr("Graph '%1' stalled!").arg(graph.objectName());
    });
    connect(&graph, &Graph::nodeAppended, this, &GraphExecutionModel::onNodeAppended);
    connect(&graph, &Graph::nodeDeleted, this, &GraphExecutionModel::onNodeDeleted);
    connect(&graph, &Graph::connectionAppended, this, &GraphExecutionModel::onConnectedionAppended);
    connect(&graph, &Graph::connectionDeleted, this, &GraphExecutionModel::onConnectionDeleted);
}

GraphExecutionModel::~GraphExecutionModel()
{
    gtTrace().verbose() << __FUNCTION__;
}

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

GraphExecutionModel::Modification
GraphExecutionModel::modify(GraphExecutionModel* model)
{
    if (!model) return {};

    return model->beginModification();
}

GraphExecutionModel::Modification
GraphExecutionModel::beginModification()
{
    if (m_isInserting) return {};

    gtTrace().verbose().nospace()
        << Impl::graphName(*this) << tr("BEGIN MODIFICIATION...");

    m_isInserting = true;
    return gt::finally(EndModificationFunctor{this});
}

void
GraphExecutionModel::endModification()
{
    m_isInserting = false;

    gtTrace().verbose().nospace()
        << Impl::graphName(*this) << tr("... END MODIFICATION");

    evaluateNextInQueue();
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
        node->setNodeFlag(NodeFlag::RequiresEvaluation);
        for (auto& data : entry.portsIn ) data.state = PortDataState::Outdated;
        for (auto& data : entry.portsOut) data.state = PortDataState::Outdated;
    }
}

void
GraphExecutionModel::endReset()
{
    m_data.clear();

    auto const& nodes = graph().nodes();
    for (auto* node : nodes)
    {
        onNodeAppended(node);
    }
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

    return find
           && find.isEvaluated()
           && find.areInputsValid();
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

    QVector<NodeId> rootNodes;

    for (auto* node : nodes)
    {
        // nodes that have no input connections are our root nodes
        if (graph.findConnections(node->id(), PortType::In).empty())
        {
            rootNodes << node->id();
        }
    }

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

    // find target nodes (nodes with not output connections)
    QVector<NodeId> targetNodes;

    for (auto const* node : nodes)
    {
        if (graph.findConnections(node->id(), PortType::Out).empty())
        {
            targetNodes.push_back(node->id());
        }
    }

    if (targetNodes.empty())
    {
        gtError().nospace()
            << Impl::graphName(*this)
            << tr("Failed to evaluate graph, target nodes not found!");
        return {};
    }

    // evaluate until target nodes
    for (NodeId target : qAsConst(targetNodes))
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
        gtDebug() << makeError() << tr("(Node is already marked for evaluation)");
        return FutureNodeEvaluated(this, nodeId);
    }

    m_targetNodes.push_back(nodeId);

    if (!evaluateNodeDependencies(nodeId))
    {
        m_targetNodes.removeLast();
        rescheduleTargetNodes();
        return {};
    }

    return FutureNodeEvaluated(this, nodeId);
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

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << makeError() << tr("(Node not found)");
        return false;
    }

    if (!find.node->isActive())
    {
        gtDebug().verbose() << makeError() << tr("(node %1 is not active)").arg(nodeId);
        return false;
    }

    if (find.node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtDebug().verbose() << makeError() << tr("(Node is already evaluating)");
        // nothing to do
        return true;
    }

    // node is evaluated -> evaluate next nodes
    if (find.isEvaluated())
    {
        gtDebug().verbose() << makeError() << tr("(Node was already evaluated)");

        auto const& targetNodes = graph.findConnectedNodes(nodeId, PortType::Out);

        bool success = targetNodes.empty();

        for (NodeId nextNode : targetNodes) success |= autoEvaluateNode(nextNode);

        return success;
    }

    return queueNodeForEvaluation(nodeId);
}

bool
GraphExecutionModel::evaluateNodeDependencies(NodeId nodeId)
{
    assert(nodeId != invalid<NodeId>());

    auto const makeError = [this, nodeId](){
        return Impl::graphName(*this) +
               tr("Failed to schedule node %1!").arg(nodeId);
    };

    if (m_pendingNodes.contains(nodeId))
    {
        gtDebug() << makeError() << tr("(Node is already pending)");
        return true;
    }

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << makeError() << tr("(Node not found)");
        return false;
    }

    if (find.node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtDebug().verbose() << makeError() << tr("(Node is already evaluating)");
        return true;
    }

    if (find.isEvaluated())
    {
        gtDebug().verbose() << makeError() << tr("(Node was already evaluated)");
        return true;
    }

    m_pendingNodes.push_back(nodeId);

    if (queueNodeForEvaluation(nodeId))
    {
        return true;
    }

    // node not ready to be queued -> dependencies not fullfilled

    auto const& dependencies = graph().findConnectedNodes(nodeId, PortType::In);

    gtDebug().verbose().nospace()
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
        gtDebug().verbose().nospace()
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

    if (m_queuedNodes.contains(nodeId))
    {
        gtDebug().verbose() << makeError() << tr("(Node is already queued)");
        return true;
    }

    auto find = Impl::findNode(*this, nodeId);
    if (!find)
    {
        gtError() << makeError() << tr("(Node not found)");
        return false;
    }

    if (!find.canEvaluate())
    {
        gtDebug().verbose()
            << makeError()
            << tr("(Node is not ready for evaluation, some inputs are %1)")
                   .arg(find.areInputsValid() ?
                            QString("required but null") :
                            QString("not valid yet"));
        return false;
    }

    gtDebug().nospace().verbose()
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

    if (m_isInserting)
    {
        gtDebug().verbose() << makeError() << tr("(Model is inserting)");
        return false;
    }

    bool scheduledNode = false;

    for (int idx = 0; idx <= m_queuedNodes.size() - 1; ++idx)
    {
        NodeId nodeId = m_queuedNodes.at(idx);

        auto* node = graph().findNode(nodeId);
        if (!node)
        {
            m_queuedNodes.removeAt(idx--);
            continue;
        }

        assert(!m_evaluatingNodes.contains(node));
        assert(!(node->nodeFlags() & NodeFlag::Evaluating));

        auto containsExclusiveNodes =
            std::any_of(m_evaluatingNodes.cbegin(), m_evaluatingNodes.cend(),
                        [](QPointer<Node> const& evaluating){
            return evaluating && evaluating->nodeEvalMode() == NodeEvalMode::Exclusive;
        });

        if (containsExclusiveNodes)
        {
            gtDebug().verbose() << makeError(node) << tr("(Executor contains exclusive nodes)");
            return false;
        }

        if (node->nodeEvalMode() == NodeEvalMode::Exclusive && !m_evaluatingNodes.empty())
        {
            gtDebug().verbose() << makeError(node) << tr("(Node is exclusive and executor is not empty)");
            continue;
        }

        m_evaluatingNodes.push_back(node);
        m_queuedNodes.removeAt(idx--);

        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("Evaluating node %1...").arg(nodeId);

        if (!Impl::doTriggerNode(*this, node))
        {
            gtError() << makeError(node) << tr("(Node execution failed)");
            // node may already be removed here
            m_evaluatingNodes.removeOne(node);
            continue;
        }

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

void
GraphExecutionModel::debug() const
{
    QVector<NodeId> evaluating;
    std::transform(m_evaluatingNodes.begin(), m_evaluatingNodes.end(), std::back_inserter(evaluating), [](QPointer<Node> const& node){ return node->id(); });

    gtDebug() << Impl::graphName(*this) << "#######################";
    gtDebug() << Impl::graphName(*this) << "target nodes:    " << m_targetNodes;
    gtDebug() << Impl::graphName(*this) << "pending nodes:   " << m_pendingNodes;
    gtDebug() << Impl::graphName(*this) << "queued nodes:    " << m_queuedNodes;
    gtDebug() << Impl::graphName(*this) << "evaluating nodes:" << evaluating;
    gtDebug() << Impl::graphName(*this) << "#######################";
}

bool
GraphExecutionModel::invalidatePort(NodeId nodeId, dm::PortEntry& port)
{
    if (port.state != PortDataState::Valid) return true; // nothing to do here

    Node* node = graph().findNode(nodeId);
    if (!node) return false;

    port.state = PortDataState::Outdated;

    gtDebug().nospace().verbose()
        << Impl::graphName(*this)
        << "Invalidating node " << nodeId << " port id " << port.id;

    PortType type = node->portType(port.id);

    switch (type)
    {
    case PortType::Out:
    {
        node->setNodeFlag(NodeFlag::RequiresEvaluation);

        bool success = true;
        for (auto& con : graph().findConnections(nodeId, PortType::Out))
        {
            success &= invalidatePort(con.inNodeId, con.inPort);
        }
        return success;
    }
    case PortType::In:
        return invalidateOutPorts(nodeId);
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }
    return false;
}

bool
GraphExecutionModel::invalidateOutPorts(NodeId nodeId)
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end()) return false;

    Node* node = graph().findNode(nodeId);
    if (!node) return false;

    node->setNodeFlag(NodeFlag::RequiresEvaluation);

    bool success = true;
    for (auto& port : entry->portsOut)
    {
        success &= invalidatePort(nodeId, port);
    }
    return success;
}

bool
GraphExecutionModel::invalidatePort(NodeId nodeId, PortId portId)
{
    auto port = Impl::findPortDataEntry(*this, nodeId, portId);
    if (!port) return false;
    
    invalidatePort(nodeId, *port);

    return true;
}

dm::NodeDataSet
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

    return *port;
}

dm::NodeDataSet
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
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, dm::NodeDataSet data)
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

    port->data = std::move(data.data);

    switch (type)
    {
    case PortType::In:
    {
        port->state = data.state;

        invalidateOutPorts(nodeId);

        emit find.node->inputDataRecieved(portId);

        if (m_autoEvaluate)
        {
            // check if source node is still evaluating -> else trigger node evaluation
            auto const& dependencies = graph.findConnectedNodes(nodeId, portId);

            bool isEvaluating = std::any_of(dependencies.begin(),
                                            dependencies.end(),
                                            [&graph](NodeId dependency){
                Node const* node = graph.findNode(dependency);
                return node && (node->nodeFlags() & NodeFlag::Evaluating);
            });

            if (!isEvaluating)
            {
                gtDebug().nospace()
                    << Impl::graphName(*this)
                    << tr("Triggering node %1 from input data").arg(nodeId);
                autoEvaluateNode(nodeId);
            }
        }
        break;
    }
    case PortType::Out:
    {
        port->state = find.areInputsValid() ? data.state :
                                              PortDataState::Outdated;

        // forward data to target nodes
        auto const& connections = graph.findConnections(nodeId, portId);

        for (ConnectionId con : connections)
        {
            setNodeData(con.inNodeId, con.inPort, *port);
        }
        break;
    }
    case PortType::NoType:
        throw GTlabException(__FUNCTION__, "path is unreachable!");
    }

    return true;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortType type, PortIndex idx, dm::NodeDataSet data)
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

    dm::Entry entry{};

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

    /// create new port entry in model
    connect(node, &Node::portInserted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
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

        (type == PortType::In ? find.entry->portsIn : find.entry->portsOut)
            .push_back({portId});
    });

    /// delete port entry from model, pause model execution
    connect(node, &Node::portAboutToBeDeleted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
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

        m_isInserting = true;

        (type == PortType::In ? find.entry->portsIn : find.entry->portsOut)
            .removeAt(find.idx);
    });

    /// when port is deleted -> evaluation can continue
    connect(node, &Node::portDeleted,
            this, [this, nodeId = node->id()](PortType, PortIndex){
                endModification();
    });
}

void
GraphExecutionModel::onNodeDeleted(NodeId nodeId)
{
    m_data.remove(nodeId);

    bool queueChanged = false;
    queueChanged |= m_evaluatingNodes.removeOne(nullptr);
    queueChanged |= m_queuedNodes.removeOne(nodeId);

    bool nodeRemoved = false;
    nodeRemoved |= m_pendingNodes.removeOne(nodeId);
    nodeRemoved |= m_targetNodes.removeOne(nodeId);

    if (nodeRemoved)
    {
        return rescheduleTargetNodes();
    }
    if (queueChanged)
    {
        return (void)evaluateNextInQueue();
    }
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

    gtDebug() << Impl::graphName(*this) << "forwarding node data";

    // set node data
    auto data = nodeData(conId.outNodeId, conId.outPort);
    setNodeData(conId.inNodeId, conId.inPort, std::move(data));

    gtDebug() << Impl::graphName(*this) << "scheduling node" << conId.outNodeId;

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
            << tr("Node %1 has evaluated, but was no longer found in the execution model!").arg(nodeId);
        return emit internalError();
    }

    assert(node == find.node);

    if (node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("Node %1 is still evaluating!").arg(nodeId);
        return;
    }

    m_evaluatingNodes.removeOne(node);

    if (node->nodeFlags() & NodeFlag::RequiresEvaluation)
    {
        gtWarning().nospace().verbose()
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
    if (find.areInputsValid())
    {
        for (auto& port : find.entry->portsOut)
        {
            if (port.state == PortDataState::Outdated)
            {
                setNodeData(nodeId, port.id, port.data);
            }
        }
    }

    m_targetNodes.removeOne(nodeId);

    emit nodeEvaluated(nodeId);

    // attempt reschedule pending nodes
    bool debug = !m_pendingNodes.empty();
    if (debug)
    {
        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("Queue pending nodes...");

        this->debug();
    }

    for (int idx = m_pendingNodes.size() - 1; idx >= 0; --idx)
    {
        NodeId pending = m_pendingNodes.at(idx);
        m_pendingNodes.removeAt(idx);

        if (!evaluateNodeDependencies(pending))
        {
            m_pendingNodes.append(pending);
        }
    }

    if (debug)
    {
        this->debug();

        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("...done!");
    }

    // queue next nodes
    if (m_autoEvaluate)
    {
        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("Triggering next nodes...");

        auto const& nextNodes = graph.findConnectedNodes(nodeId, PortType::Out);

        for (NodeId nextNode : nextNodes)
        {
            autoEvaluateNode(nextNode);
        }

        gtDebug().nospace().verbose()
            << Impl::graphName(*this)
            << tr("...done!");
    }

    // trigger evaluation
    if (!evaluateNextInQueue() &&
        m_evaluatingNodes.empty() &&
        m_queuedNodes.empty() &&
        !m_pendingNodes.empty())
    {
        emit graphStalled();
    }

    // DEBUG
    if (!m_autoEvaluate && m_targetNodes.empty())
    {
        std::for_each(m_data.keyValueBegin(), m_data.keyValueEnd(),
                      [&graph, this](std::pair<NodeId, dm::Entry&> const& p){
            auto* node = graph.findNode(p.first);
            assert(node);
            gtDebug()
                << Impl::graphName(*this)
                << node->id()
                << !(node->nodeFlags() & NodeFlag::Evaluating)
                << !(node->nodeFlags() & NodeFlag::RequiresEvaluation)
                << p.second.areInputsValid(graph, p.first);
        });
    }

    if (isEvaluated()) emit graphEvaluated();
}
