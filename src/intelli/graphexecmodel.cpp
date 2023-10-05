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

dm::NodeData
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

dm::NodeData
DummyDataModel::nodeData(PortId portId, dm::NodeData data)
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
DummyDataModel::setNodeData(NodeId nodeId, PortId portId, dm::NodeData data)
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
DummyDataModel::setNodeData(PortId portId, dm::NodeData data)
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

struct GraphExecutionModel::Impl
{

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

    operator T*() { return port; }
    operator T const*() const { return port; }

    operator PortHelper<T const>() const { return {port, type}; }
};

static PortHelper<dm::PortEntry>
findPortDataEntry(GraphExecutionModel& model, NodeId nodeId, PortId portId)
{
    auto entry = model.m_data.find(nodeId);
    if (entry == model.m_data.end())
    {
        gtError() << model.graph().objectName() + ':'
                  << tr("Failed to access port data! (Invalid node %1)").arg(nodeId);
        return {};
    }

    for (auto* ports : {&entry->portsIn, &entry->portsOut})
    {
        for (auto& port : *ports)
        {
            if (port.id == portId) return { &port, portType(*entry, *ports) };
        }
    }

    return { nullptr, PortType::NoType };
}

static PortHelper<dm::PortEntry const>
findPortDataEntry(GraphExecutionModel const& model, NodeId nodeId, PortId portId)
{
    return findPortDataEntry(const_cast<GraphExecutionModel&>(model), nodeId, portId);
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

    connect(this, &GraphExecutionModel::nodeEvaluated, this, [&graph](NodeId nodeId){
        gtDebug() << graph.objectName() + ':'
                  << tr("Node %1 evaluated!").arg(nodeId);
    });
    connect(this, &GraphExecutionModel::graphEvaluated, this, [&graph](){
        gtDebug() << tr("Graph '%1' evaluated!").arg(graph.objectName());
    });
    connect(this, &GraphExecutionModel::graphStalled, this, [&graph](){
        gtWarning() << tr("Graph '%1' stalled!").arg(graph.objectName());
    });
    connect(&graph, &Graph::nodeAppended, this, &GraphExecutionModel::appendNode);
    connect(&graph, &Graph::nodeDeleted, this, &GraphExecutionModel::onNodeDeleted);
    connect(&graph, &Graph::connectionAppended, this, &GraphExecutionModel::onConnectedionAppended);
    connect(&graph, &Graph::connectionDeleted, this, &GraphExecutionModel::onConnectionDeleted);
}

GraphExecutionModel::~GraphExecutionModel()
{
    gtTrace().verbose() << __FUNCTION__;
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

GraphExecutionModel::Insertion
GraphExecutionModel::beginInsertion()
{
    m_isInserting = true;
    return gt::finally(EndInsertionFunctor{this});
}

void
GraphExecutionModel::endInsertion()
{
    m_isInserting = false;

    // restart auto evaluation
    autoEvaluate(m_autoEvaluate);
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
    m_targetNodeId = NodeId();

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
        appendNode(node);
    }
}

bool
GraphExecutionModel::evaluated()
{
    auto& graph = this->graph();
    return std::all_of(m_data.keyValueBegin(), m_data.keyValueEnd(),
                       [&graph](std::pair<NodeId, dm::Entry&> const& p){
        auto* node = graph.findNode(p.first);
        return node
               && !(node->nodeFlags() & (NodeFlag::Evaluating | NodeFlag::RequiresEvaluation))
               && p.second.areInputsValid(graph, p.first);
    });
}

bool
GraphExecutionModel::wait(std::chrono::milliseconds timeout)
{
    if (evaluated()) return true;

    if (!m_autoEvaluate)
    {
        gtError() << graph().objectName() + ':'
                  << tr("Cannot wait for graph evaluation if auto evaluation is not enabled!");
        return false;
    }

    GtEventLoop eventLoop{timeout};

    if (timeout == timeout.max()) eventLoop.setTimeout(-1);

    eventLoop.connectSuccess(this, &GraphExecutionModel::graphEvaluated);
    eventLoop.connectFailed(this, &GraphExecutionModel::internalError);

    return eventLoop.exec() == GtEventLoop::Success;
}

bool
GraphExecutionModel::waitForNode(std::chrono::milliseconds timeout)
{
    if (m_targetNodeId == invalid<NodeId>())
    {
        gtError() << graph().objectName() + ':'
                  << tr("Cannot wait for node! (No node was set as active)");
        return false;
    }

    auto entry = m_data.find(m_targetNodeId);
    if (entry == m_data.end())
    {
        gtError() << graph().objectName() + ':'
                  << tr("Cannot wait for node! (Target node %1 not found)").arg(m_targetNodeId);
        return false;
    }

    auto* targetNode = graph().findNode(m_targetNodeId);
    if (!targetNode)
    {
        gtError() << graph().objectName() + ':'
                  << tr("Cannot wait for node! (Target node object of node %1 not found)").arg(m_targetNodeId);
        return false;
    }

    if (entry->isEvaluated(*targetNode)) return true;

    GtEventLoop eventLoop{timeout};

    if (timeout == timeout.max()) eventLoop.setTimeout(-1);

    eventLoop.connectFailed(this, &GraphExecutionModel::internalError);
    eventLoop.connectFailed(this, &GraphExecutionModel::graphStalled);

    connect(this, &GraphExecutionModel::nodeEvaluated,
            &eventLoop, [&eventLoop, this](NodeId nodeId){
        if (nodeId == m_targetNodeId)
        {
            emit eventLoop.success();
        }
    });

    return eventLoop.exec() == GtEventLoop::Success;
}

bool
GraphExecutionModel::triggerNodeExecution(NodeId nodeId)
{
    auto& graph = this->graph();

    auto makeError = [&graph](){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Failed to trigger node execution!");
    };

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << makeError() << tr("(node %1 not found)").arg(nodeId);
        return false;
    }

    auto* node = graph.findNode(nodeId);
    if (!node)
    {
        gtError() << makeError() << tr("(node object for node id %1 not found)").arg(nodeId);
        return false;
    }

    if (!node->isActive())
    {
        gtDebug() << makeError()
                  << tr("(node %1 is not active)").arg(nodeId);
        return false;
    }

    if (node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtDebug() << makeError() << tr("(node %1 already evaluating)").arg(nodeId);
        return false;
    }

    if (!entry->canEvaluate(graph, *node))
    {
        gtDebug() << makeError() << tr("(node %1 is not ready yet)").arg(nodeId);
        return false;
    }

    connect(node, &Node::computingFinished,
            this, &GraphExecutionModel::onNodeEvaluated,
            Qt::UniqueConnection);

    bool evaluatedOnce = false;

    auto cleanup = gt::finally([&evaluatedOnce, node, this](){
        if (!evaluatedOnce)
        {
            disconnect(node, &Node::computingFinished,
                       this, &GraphExecutionModel::onNodeEvaluated);
        }
    });

    // find ports to evaluate
    QVector<PortId> portsToEvaluate;
    QSet<NodeId> targetNodes;

    for (auto& port : qAsConst(entry->portsOut))
    {
        if (port.state != PortDataState::Valid)
        {
            portsToEvaluate.push_back(port.id);
        }
        
        auto const& nodes = graph.findConnectedNodes(nodeId, port.id);

        for (NodeId n : nodes) targetNodes.insert(n);
    }

    // all ports need evaluating
    if (portsToEvaluate.size() == entry->portsOut.size() ||
        node->nodeFlags() & NodeFlag::RequiresEvaluation)
    {
        if (entry->portsOut.empty() && !(node->nodeFlags() & NodeFlag::RequiresEvaluation)) return false;

        evaluatedOnce |= node->handleNodeEvaluation(*this, invalid<PortId>());
        return evaluatedOnce;
    }

    // evaluate ports individually
    for (auto& port : portsToEvaluate)
    {
        evaluatedOnce |= node->handleNodeEvaluation(*this, port);
    }

    cleanup.finalize();

    if (!m_autoEvaluate) return evaluatedOnce;

    // trigger all target nodes, that have not been triggered automatically
    for (NodeId nextNode : targetNodes)
    {
        triggerNodeExecution(nextNode);
    }

    return evaluatedOnce;
}

bool
GraphExecutionModel::triggerNode(NodeId nodeId, PortId portId)
{
    auto& graph = this->graph();

    auto makeError = [&graph, nodeId](){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Failed to trigger execution of node %1!").arg(nodeId);
    };

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << makeError() << tr("(node not found)");
        return false;
    }

    Node* node = graph.findNode(nodeId);
    if (!node)
    {
        gtError() << makeError() << tr("(node object not found)");
        return false;
    }

    if (entry->isEvaluated(*node))
    {
        // nothing to do
        gtDebug().verbose() << tr("Node %1 was already evaluated").arg(nodeId);
        return true;
    }

    if (node->nodeFlags() & NodeFlag::Evaluating) return true;

    // all possible inputs are set
    if (entry->areInputsValid(graph, nodeId))
    {
        if (!entry->canEvaluate(graph, *node))
        {
            gtError() << makeError() << tr("(node is not ready for evaluation)");
            return false;
        }
        gtDebug().verbose() << tr("Node %1 ready for evaluation").arg(nodeId);
        return triggerNodeExecution(nodeId);
    }

    gtDebug().verbose() << tr("Node %1 not ready for evaluation. Checking dependencies...").arg(nodeId);

    // node dependencies not fullfilled
    auto const& dependencies = graph.findConnections(nodeId, PortType::In);

    if (dependencies.empty())
    {
        gtError() << makeError() << tr("(node has no input connections but is not ready for evaluation)");
        return false;
    }

    for (auto conId : dependencies)
    {
        if (!triggerNode(conId.outNodeId, conId.outPort)) return false;
    }

    return true;
}

bool
GraphExecutionModel::autoEvaluate(bool enable)
{
    m_autoEvaluate = enable;

    if (!enable) return true;

    auto& graph = this->graph();

    auto const& cyclicNodes = intelli::cyclicNodes(graph);
    if (!cyclicNodes.empty())
    {
        gtError() << graph.objectName() + ':'
                  << tr("Cannot auto evaluate cyclic graph! The node sequence")
                  << cyclicNodes << tr("contains a cycle!");

        m_autoEvaluate = false;
        return false;
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
        gtError() << graph.objectName() + ':'
                  << tr("Failed to find root nodes to begin graph evaluation!");
        m_autoEvaluate = false;
        return false;
    }

    for (NodeId nodeId : qAsConst(rootNodes))
    {
        triggerNodeExecution(nodeId);
    }

    return true;
}

bool
GraphExecutionModel::isAutoEvaluating() const
{
    return m_autoEvaluate;
}

bool
GraphExecutionModel::evaluateNode(NodeId nodeId, ExecMode mode)
{
    if (nodeId == invalid<NodeId>()) return false;

    if (m_targetNodeId == nodeId) return true;

    if (m_targetNodeId != invalid<NodeId>())
    {
        gtError() << tr("Failed to evaluate target node %1, a noe is already evaluating!").arg(nodeId);
        return false;
    }

    m_targetNodeId = nodeId;

    auto& graph = this->graph();

    auto makeError = [&graph](){
        return graph.objectName() + QStringLiteral(": ") +
               tr("Failed to trigger node execution!");
    };

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << makeError() << tr("(node %1 not found)").arg(nodeId);
        return false;
    }

    Node* node = graph.findNode(nodeId);
    if (!node)
    {
        gtError() << makeError() << tr("(node object for node id %1 not found)").arg(nodeId);
        return false;
    }

    evaluateTargetNode();
    return true;
}

void
GraphExecutionModel::invalidatePort(NodeId nodeId, dm::PortEntry& port)
{
    if (port.state != PortDataState::Valid) return; // nothing to do here

    Node* node = graph().findNode(nodeId);
    if (!node) return;

    port.state = PortDataState::Outdated;

    gtDebug() << graph().objectName() + ':'
              << "Invalidating node" << nodeId << "port id" << port.id;

    PortType type = node->portType(port.id);

    switch (type)
    {
    case PortType::Out:
        node->invalidate();

        for (auto& con : graph().findConnections(nodeId, PortType::Out))
        {
            invalidatePort(con.inNodeId, con.inPort);
        }
        break;
    case PortType::In:
        invalidateOutPorts(nodeId);
        break;
    }
}

bool
GraphExecutionModel::invalidateOutPorts(NodeId nodeId)
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end()) return false;

    Node* node = graph().findNode(nodeId);
    if (!node) return false;

    node->invalidate();

    for (auto& port : entry->portsOut)
    {
        invalidatePort(nodeId, port);
    }
    return true;
}

bool
GraphExecutionModel::invalidatePort(NodeId nodeId, PortId portId)
{
    auto port = Impl::findPortDataEntry(*this, nodeId, portId);
    if (!port) return false;
    
    invalidatePort(nodeId, *port);

    return true;
}

dm::NodeData
GraphExecutionModel::nodeData(NodeId nodeId, PortId portId) const
{
    auto port = Impl::findPortDataEntry(*this, nodeId, portId);

    if (!port)
    {
        gtWarning() << graph().objectName() + ':'
                    << tr("Accessing data of node %1 failed! (Port id %2 not found)")
                           .arg(nodeId).arg(portId);
        return {};
    }

    return *port;
}

dm::NodeData
GraphExecutionModel::nodeData(NodeId nodeId, PortType type, PortIndex idx) const
{
    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtWarning() << graph().objectName() + ':'
                    << tr("Accessing data of node %1 failed! (Node not found)")
                           .arg(nodeId);
        return {};
    }

    return nodeData(nodeId, node->portId(type, idx));
}

NodeDataPtrList
GraphExecutionModel::nodeData(NodeId nodeId, PortType type) const
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << graph().objectName() + ':'
                  << tr("Failed to access node data! (Invalid node %1)").arg(nodeId);
        return {};
    }

    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtError() << graph().objectName() + ':'
                  << tr("Failed to access port data! (Null node %1)").arg(nodeId);
        return {};
    }

    NodeDataPtrList data;

    auto& ports = Impl::ports(*entry, type);
    for (auto& port : ports)
    {
        data.push_back({node->portIndex(type, port.id), port.data});
    }
    return data;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, dm::NodeData data)
{
    return setNodeData(nodeId, portId, std::move(data), NoOption);
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, dm::NodeData data, int option)
{
    auto makeError = [this, nodeId](){
        return graph().objectName() + QStringLiteral(": ") +
               tr("Setting data for node %1 failed!").arg(nodeId);
    };

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtWarning() << makeError() << tr("(Node entry not found)");
        return false;
    }
    auto result = Impl::findPortDataEntry(*this, nodeId, portId);
    auto port = result.port;
    auto type = result.type;
    if (!port)
    {
        gtWarning() << makeError() << tr("(Port %1 not found)").arg(portId);
        return false;
    }

    auto& graph = this->graph();

    auto* node = graph.findNode(nodeId);
    if (!node)
    {
        gtError() << makeError() << tr("(node object not found)");
        return false;
    }

    port->data = std::move(data.data);

    switch (type)
    {
    case PortType::In:
    {
        port->state = data.state;

        invalidateOutPorts(nodeId);

        emit node->inputDataRecieved(portId);

        if (m_autoEvaluate && !(node->nodeFlags() & NodeFlag::Evaluating) && !(option & Option::DoNotTrigger))
        {
            gtDebug() << graph.objectName() + ':'
                      << tr("Triggering node %1 from input data").arg(nodeId);
            triggerNodeExecution(nodeId);
        }
        break;
    }
    case PortType::Out:
    {
        port->state = entry->areInputsValid(graph, nodeId) ? data.state : PortDataState::Outdated;

        // forward data to target nodes
        auto const& connections = graph.findConnections(nodeId, portId);

        for (ConnectionId con : connections)
        {
            int option = Option::NoOption;
            if (node->nodeFlags() & NodeFlag::Evaluating) option |= Option::DoNotTrigger;

            setNodeData(con.inNodeId, con.inPort, *port, option);
        }
        break;
    }
    case PortType::NoType:
        throw std::logic_error("path is unreachable!");
    }

    return true;
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortType type, PortIndex idx, dm::NodeData data, int option)
{
    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtWarning() << graph().objectName() + ':'
                    << tr("Setting data of node %1 failed! (Node not found)")
                           .arg(nodeId);
        return {};
    }

    return setNodeData(nodeId, node->portId(type, idx), std::move(data), option);
}

bool
GraphExecutionModel::setNodeData(NodeId nodeId,
                                 PortType type,
                                 NodeDataPtrList const& data,
                                 int option)
{
    PortIndex idx(0);
    for (auto d : data)
    {
        int opt = option;
        if (idx != data.size() - 1) opt |= Option::DoNotTrigger;

        if (!setNodeData(nodeId, type, d.first, std::move(d.second), opt))
        {
            return false;
        }
        idx++;
    }

    return true;
}

void
GraphExecutionModel::appendNode(Node* node)
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

    connect(node, &Node::triggerPortEvaluation,
            this, [this, nodeId = node->id()](PortId portId){

        portId == invalid<PortId>() ? invalidateOutPorts(nodeId) :
                                      invalidatePort(nodeId, portId);

        if (m_autoEvaluate) triggerNodeExecution(nodeId);
    });
}

void
GraphExecutionModel::onNodeDeleted(NodeId nodeId)
{
    m_data.remove(nodeId);
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

    auto entry = m_data.find(conId.outNodeId);
    if (entry == m_data.end())
    {
        gtError() << makeError()
                  << tr("(out node %1 not found)").arg(conId.outNodeId);
        return;
    }

    auto* node = graph().findNode(conId.outNodeId);
    if (!node)
    {
        gtError() << makeError()
                  << tr("(object for out node %1 not found)").arg(conId.outNodeId);
        return;
    }

    if (m_autoEvaluate && m_isInserting) return;

    int option = Option::NoOption;
    if (node->nodeFlags() & NodeFlag::Evaluating) option |= Option::DoNotTrigger;
    
    auto data = nodeData(conId.outNodeId, conId.outPort);
    setNodeData(conId.inNodeId, conId.inPort, std::move(data), option);
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionId conId)
{
    invalidatePort(conId.inNodeId, conId.inPort);
}

void
GraphExecutionModel::onNodeEvaluated()
{
    auto const& graph = this->graph();

    auto* node = qobject_cast<Node*>(sender());
    if (!node)
    {
        gtError() << graph.objectName() + ':'
                  << tr("A node has evaluated, but its object was not found!");
        return emit internalError();
    }

    if (node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtWarning() << graph.objectName() + ':'
                    << tr("Node %1 is still evaluated!").arg(node->id());
        return;
    }

    if (node->nodeFlags() & NodeFlag::Evaluating)
    {
        gtDebug().verbose()
            << graph.objectName() + ':'
            << tr("Node %1 requires reevaluation!").arg(node->id());
        triggerNodeExecution(node->id());
        return;
    }

    disconnect(node, &Node::computingFinished,
               this, &GraphExecutionModel::onNodeEvaluated);

    NodeId nodeId = node->id();

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << graph.objectName() + ':'
                  << tr("Node %1 has evaluated, but was no longer found in the exec model!").arg(nodeId);
        return emit internalError();
    }

    emit nodeEvaluated(nodeId);
    evaluateTargetNode(nodeId);

    if (m_autoEvaluate)
    {
        // forward data to target nodes
        auto const& targetNodes = graph.findConnectedNodes(nodeId, PortType::Out);

        for (NodeId nextNode : targetNodes)
        {
            triggerNodeExecution(nextNode);
        }

        if (evaluated()) emit graphEvaluated();
    }
}

void
GraphExecutionModel::evaluateTargetNode(NodeId nodeId)
{
    if (m_targetNodeId == invalid<NodeId>()) return;

    // done
    if (m_targetNodeId == nodeId)
    {
        gtDebug().verbose() << tr("Target node %1 evaluated!").arg(nodeId);
        m_targetNodeId = invalid<NodeId>();
        return;
    }

    if (!triggerNode(m_targetNodeId))
    {
        m_targetNodeId = invalid<NodeId>();
        emit graphStalled();
    }
}
