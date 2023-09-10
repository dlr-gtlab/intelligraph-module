/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/graphexecmodel.h"

#include "intelli/graph.h"
#include "intelli/connection.h"

#include "intelli/private/utils.h"

#include <gt_exceptions.h>
#include <gt_eventloop.h>

#include <gt_logging.h>

using namespace intelli;


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

static PortHelper<GraphExecutionModel::PortDataEntry>
findPortDataEntry(GraphExecutionModel& model, NodeId nodeId, PortId portId)
{
    auto entry = model.m_data.find(nodeId);
    if (entry == model.m_data.end())
    {
        gtError() << model.graph().objectName() + ':'
                  << tr("Failed to access port data! (Invalid node %1)").arg(nodeId);
        return {};
    }

    PortType type = PortType::In;
    for (auto* ports : {&entry->portsIn, &entry->portsOut})
    {
        for (auto& port : *ports)
        {
            if (port.id == portId) return { &port, type };
        }
        type = PortType::Out;
    }

    return { nullptr, PortType::NoType };
}

static PortHelper<GraphExecutionModel::PortDataEntry const>
findPortDataEntry(GraphExecutionModel const& model, NodeId nodeId, PortId portId)
{
    return findPortDataEntry(const_cast<GraphExecutionModel&>(model), nodeId, portId);
}

};

//////////////////////////////////////////////////////

GraphExecutionModel::GraphExecutionModel(Graph& graph)
{
    setParent(&graph);

    reset();

    connect(this, &GraphExecutionModel::nodeEvaluated, this, [&graph](NodeId nodeId){
        gtDebug() << graph.objectName() + ':'
                  << tr("Node %1 evaluated!").arg(nodeId);
    });
    connect(this, &GraphExecutionModel::graphEvaluated, this, [&graph](){
        gtDebug() << tr("Graph '%1' evaluated!").arg(graph.objectName());
    });
    connect(&graph, &Graph::nodeAppended, this, &GraphExecutionModel::appendNode);
    connect(&graph, &Graph::nodeDeleted, this, &GraphExecutionModel::onNodeDeleted);
    connect(&graph, &Graph::connectionAppended, this, &GraphExecutionModel::onConnectedionAppended);
    connect(&graph, &Graph::connectionDeleted, this, &GraphExecutionModel::onConnectionDeleted);
}

Graph&
GraphExecutionModel::graph()
{
    auto tmp = static_cast<Graph*>(parent());
    assert(qobject_cast<Graph*>(parent()));
    return *tmp;
}

const Graph&
GraphExecutionModel::graph() const
{
    return const_cast<GraphExecutionModel*>(this)->graph();
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
    for (Entry& entry : m_data)
    {
        entry.state = NodeEvalState::Evaluated;
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
    return std::all_of(m_data.begin(), m_data.end(), [=](Entry const& entry){
        return entry.state == NodeEvalState::Evaluated && entry.isDataValid();
    });
}

bool
GraphExecutionModel::wait(std::chrono::milliseconds timeout)
{
    GtEventLoop eventLoop{timeout};

    if (timeout == timeout.zero()) eventLoop.setTimeout(-1);

    eventLoop.connectSuccess(this, & GraphExecutionModel::graphEvaluated);

    if (evaluated()) return true;

    if (!m_autoEvaluate)
    {
        gtError() << graph().objectName() + ':'
                  << tr("Cannot wait for graph evaluation if auto evaluation is not enabled!");
        return false;
    }

    return eventLoop.exec() == GtEventLoop::Success;
}

void
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
        return;
    }

    if (entry->state != NodeEvalState::Evaluated)
    {
        gtDebug() << makeError() << tr("(node %1 already evaluatingn)").arg(nodeId);
        invalidateOutPorts(nodeId);
        return;
    }

    auto* node = graph.findNode(nodeId);
    if (!node)
    {
        gtError() << makeError() << tr("(node object for node id %1 not found)").arg(nodeId);
        return;
    }

    if (!entry->canEvaluate(*node))
    {
        gtDebug() << makeError() << tr("(node %1 is not ready yet)").arg(nodeId);
        return;
    }

//    gtDebug() << graph.objectName() + ':' << tr("evaluating node") << nodeId;

    entry->state = NodeEvalState::Evaluating;

    connect(node, &Node::computingFinished,
            this, &GraphExecutionModel::onNodeEvaluated,
            Qt::UniqueConnection);

    bool evaluatedOnce = false;

    auto cleanup = gt::finally([&evaluatedOnce, entry, node, this](){
        if (!evaluatedOnce)
        {
            entry->state = NodeEvalState::Evaluated;

            disconnect(node, &Node::computingFinished,
                       this, &GraphExecutionModel::onNodeEvaluated);
        }
    });

    // find ports to evaluate
    QVector<PortId> portsToEvaluate;
    QSet<NodeId> targetNodes;

    for (auto& port : qAsConst(entry->portsOut))
    {
        if (!port.isValid())
        {
            portsToEvaluate.push_back(port.id);
        }

        auto const& nodes = graph.findTargetNodes(nodeId, PortType::Out, port.id);

        for (NodeId n : nodes) targetNodes.insert(n);
    }

    // all ports need evauating
    if (portsToEvaluate.size() == entry->portsOut.size())
    {
        evaluatedOnce |= node->handleNodeEvaluation(*this, invalid<PortId>());
        return;
    }

    // evaluate ports individually
    for (auto& port : portsToEvaluate)
    {
        evaluatedOnce |= node->handleNodeEvaluation(*this, port);
    }

    cleanup.finalize();

    if (!m_autoEvaluate) return;

    // trigger all target nodes, that have not been triggered automatically
    for (NodeId nextNode : targetNodes)
    {
//        gtDebug() << graph.objectName() + ':'
//                  << "triggering next node" << nextNode;
        triggerNodeExecution(nextNode);
    }
}

bool
GraphExecutionModel::autoEvaluate(bool enable)
{
    m_autoEvaluate = enable;

    if (!enable) return true;

    auto& graph = this->graph();

    auto const& cyclicNodes = intelli::cyclicNodes(graph);
    if (!cyclicNodes.isEmpty())
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

void
GraphExecutionModel::invalidatePort(NodeId nodeId, PortDataEntry& port)
{
    if (!port.isValid()) return; // nothing to do here

    port.state = PortDataState::Outdated;

    auto const& connections = graph().findConnections(nodeId, PortType::Out);

    gtDebug() << graph().objectName() + ':'
              << "INVALIDATING Node" << nodeId << "port index" << port.id
              << std::vector<ConnectionId>(connections.begin(), connections.end());

    for (auto& con : connections)
    {
        invalidatePort(con.inNodeId, con.inPort);
    }
}

bool
GraphExecutionModel::invalidateOutPorts(NodeId nodeId)
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end()) return false;

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

IndexedNodeData
GraphExecutionModel::nodeData(NodeId nodeId, PortType type) const
{
    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << graph().objectName() + ':'
                  << tr("Failed to access port data! (Invalid node %1)").arg(nodeId);
        return {};
    }

    auto* node = graph().findNode(nodeId);
    if (!node)
    {
        gtError() << graph().objectName() + ':'
                  << tr("Failed to access port data! (Null node %1)").arg(nodeId);
        return {};
    }

    IndexedNodeData data;

    auto& ports = Impl::ports(*entry, type);
    for (auto& port : ports)
    {
        data.push_back({node->portIndex(type, port.id), port.data});
    }
    return data;
}

GraphExecutionModel::NodeModelData
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

GraphExecutionModel::NodeModelData
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

bool
GraphExecutionModel::setNodeData(NodeId nodeId, PortId portId, NodeModelData data, int option)
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
        gtWarning() << makeError() << tr("(Port index %1 out of bounds)").arg(portId);
        return false;
    }

    port->data = std::move(data.data);

    switch (type)
    {
    case PortType::In:
    {
        port->state = data.state;

        invalidateOutPorts(nodeId);

        if (auto* node = graph().findNode(nodeId))
        {
            emit node->inputDataRecieved(portId);
        }

        if (m_autoEvaluate && entry->state == NodeEvalState::Evaluated && !(option & Option::DoNotTrigger))
        {
            gtDebug() << graph().objectName() + ':'
                      << tr("Triggering node %1 from input data").arg(nodeId);
            triggerNodeExecution(nodeId);
        }
        break;
    }
    case PortType::Out:
    {
        port->state = entry->isDataValid(PortType::In) ? data.state : PortDataState::Outdated;

        // forward data to target nodes
        auto const& connections = graph().findConnections(nodeId, PortType::Out, portId);

        for (ConnectionId con : connections)
        {
            int option = Option::NoOption;
            if (entry->state != NodeEvalState::Evaluated) option |= Option::DoNotTrigger;

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
GraphExecutionModel::setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeModelData data, int option)
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
                                 IndexedNodeData const& data,
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
    gtDebug() << __FUNCTION__;
    assert(node);

    Entry entry{};

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
}

void
GraphExecutionModel::onNodeDeleted(NodeId nodeId)
{
    gtDebug() << __FUNCTION__;
    m_data.remove(nodeId);
}

void
GraphExecutionModel::onConnectedionAppended(Connection* con)
{
    gtDebug() << __FUNCTION__;
    assert(con);
    ConnectionId conId = con->connectionId();

    auto entry = m_data.find(conId.outNodeId);
    if (entry == m_data.end())
    {
        gtError()
            << graph().objectName() << ':'
            << tr("Failed to integrate new connection %1!").arg(toString(conId))
            << tr("(out node %1 not found)").arg(conId.outNodeId);
        return;
    }

    invalidatePort(conId.inNodeId, conId.inPort);

    int option = Option::NoOption;
    if (entry->state != NodeEvalState::Evaluated) option |= Option::DoNotTrigger;
    
    auto data = nodeData(conId.outNodeId, conId.outPort);
    setNodeData(conId.inNodeId, conId.inPort, std::move(data), option);
}

void
GraphExecutionModel::onConnectionDeleted(ConnectionId conId)
{
    gtDebug() << __FUNCTION__;
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
                  << tr("A node has evaluated, but was no longer found!");
        return;
    }

    disconnect(node, &Node::computingFinished,
               this, &GraphExecutionModel::onNodeEvaluated);

    NodeId nodeId = node->id();

    auto entry = m_data.find(nodeId);
    if (entry == m_data.end())
    {
        gtError() << graph.objectName() + ':'
                  << tr("Node %1 has evaluated, but was no longer found in the model!").arg(nodeId);
        return;
    }

    entry->state = NodeEvalState::Evaluated;

    emit nodeEvaluated(nodeId);

    if (m_autoEvaluate)
    {
        // forward data to target nodes
        auto const& targetNodes = graph.findTargetNodes(nodeId, PortType::Out);

        for (NodeId nextNode : targetNodes)
        {
            //            gtDebug() << graph.objectName() + ':'
            //                      << "triggering next node" << nextNode;
            triggerNodeExecution(nextNode);
        }

        if (evaluated()) emit graphEvaluated();
    }
}
