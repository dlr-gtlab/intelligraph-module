/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/graph.h"
#include "intelli/graphdatamodel.h"
#include "intelli/graphexecmodel.h"
#include "intelli/graphexecutor.h"
#include "intelli/graphuservariables.h"
#include "intelli/private/graph_impl.h"

#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/data/invalid.h"
#include "intelli/node/dummy.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/gui/guidata.h"
#include "gt_eventloop.h"

#include <gt_qtutilities.h>
#include <gt_algorithms.h>
#include <gt_mdiitem.h>
#include <gt_mdilauncher.h>

using namespace intelli;


Graph::Graph(QString const& modelName, bool initProviders) :
    DynamicNode(modelName),
    pimpl(std::make_unique<Impl>())
{
    auto* db = new GraphUserVariables(this);
    db->setDefault(true);

    // we create the node connections here in this group object. This way
    // merging mementos has the correct order (first the connections are removed
    // then the nodes)
    auto* connectionGroup = new ConnectionGroup(this);
    connectionGroup->setDefault(true);

    auto* guiData = new GuiData(this);
    guiData->setDefault(true);

    setNodeEvalMode(NodeEvalMode::Blocking);

    connect(connectionGroup, &ConnectionGroup::mergeConnections, this, [this](){
        restoreConnections();
    });

    if (initProviders)
    {
        NodeId nextId{0};

        auto input = std::make_unique<GraphInputProvider>();
        input->setDefault(true);
        input->setUserHidden(!gtApp || !gtApp->devMode());
        input->setUserHidden(true);
        input->setId(nextId++);
        synchronizePorts(*input);
        appendNode(std::move(input), NodeIdPolicy::Keep);

        auto output = std::make_unique<GraphOutputProvider>();
        output->setDefault(true);
        output->setUserHidden(!gtApp || !gtApp->devMode());
        output->setUserHidden(true);
        output->setId(nextId++);
        synchronizePorts(*output);
        appendNode(std::move(output), NodeIdPolicy::Keep);

        // sync dynamic ports
//        connect(this, &Node::portInserted,
//                this, &Graph::onPortInserted,
//                Qt::DirectConnection);
//        connect(this, &Node::portChanged,
//                this, &Graph::onPortChanged,
//                Qt::DirectConnection);
//        connect(this, &Node::portAboutToBeDeleted,
//                this, &Graph::onPortDeleted,
//                Qt::DirectConnection);
    }
}

Graph::Graph() :
    Graph(QStringLiteral("Graph"), true)
{
    pimpl->forwardInvalidation = true;
}

Graph::~Graph()
{
    emit graphAboutToBeDeleted(QPrivateSignal());
}

Graph*
intelli::Graph::accessGraph(Node& node)
{
    return qobject_cast<Graph*>(node.parent());
}

Graph const*
intelli::Graph::accessGraph(Node const& node)
{
    return qobject_cast<Graph const*>(node.parent());
}

Graph*
Graph::parentGraph()
{
    return accessGraph(*this);
}

Graph const*
Graph::parentGraph() const
{
    return accessGraph(*this);
}

Graph*
Graph::rootGraph()
{
    // recursive search
    if (auto p = parentGraph())
    {
        return p->rootGraph();
    }
    return this;
}

Graph const*
Graph::rootGraph() const
{
    return const_cast<Graph*>(this)->rootGraph();
}


QList<Node*>
Graph::nodes()
{
    return findDirectChildren<Node*>();
}

QList<Node const*>
Graph::nodes() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->nodes()
    );
}

QVector<NodeId>
Graph::nodeIds() const
{
    QVector<NodeId> ids;
    auto const& nodes = this->nodes();
    ids.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), ids.begin(),
                   [](Node const* node){ return node->id(); });
    return ids;
}

ConnectionModel const&
Graph::connectionModel() const
{
    return pimpl->local;
}

GlobalConnectionModel const&
Graph::globalConnectionModel() const
{
    return *pimpl->global;
}

QList<Connection*>
Graph::connections()
{
    return connectionGroup().findDirectChildren<Connection*>();
}

QList<Connection const*>
Graph::connections() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->connections()
    );
}

QVector<ConnectionId>
Graph::connectionIds() const
{
    QVector<ConnectionId> ids;
    auto const& connections = this->connections();
    ids.resize(connections.size());
    std::transform(connections.begin(), connections.end(), ids.begin(),
                   [](Connection const* con){ return con->connectionId(); });
    return ids;
}

ConnectionGroup&
Graph::connectionGroup()
{
    auto* group = findDirectChild<ConnectionGroup*>();
    assert(group);
    return *group;
}

ConnectionGroup const&
Graph::connectionGroup() const
{
    return const_cast<Graph*>(this)->connectionGroup();
}

GraphInputProvider*
Graph::inputProvider()
{
    return findDirectChild<GraphInputProvider*>();
}

GraphInputProvider const*
Graph::inputProvider() const
{
    return const_cast<Graph*>(this)->inputProvider();
}

Node*
Graph::inputNode()
{
    return inputProvider();
}

Node const*
Graph::inputNode() const
{
    return inputProvider();
}

GraphOutputProvider*
Graph::outputProvider()
{
    return this->findDirectChild<GraphOutputProvider*>();
}

GraphOutputProvider const*
Graph::outputProvider() const
{
    return const_cast<Graph*>(this)->outputProvider();
}

Node*
Graph::outputNode()
{
    return outputProvider();
}

Node const*
Graph::outputNode() const
{
    return outputProvider();
}

QVector<NodeId>
Graph::findDependencies(NodeId nodeId) const
{
    QVector<NodeId> nodes;
    if (!Impl::accumulateDependentNodes(*this, nodes, nodeId, PortType::In))
    {
        return {};
    }
    return nodes;
}

QVector<NodeId>
Graph::findDependentNodes(NodeId nodeId) const
{
    QVector<NodeId> nodes;
    if (!Impl::accumulateDependentNodes(*this, nodes, nodeId, PortType::Out))
    {
        return {};
    }
    return nodes;
}

Node*
Graph::findNode(NodeId nodeId)
{
    auto iter = pimpl->local.find(nodeId);
    if (iter == pimpl->local.end()) return {};

    if (!iter->node)
    {
        gtWarning().verbose()
            << gt::log::nospace
            << utils::logIds(this)
            << "::findNode(" << nodeId << ")\n"
            << "-> found node: " << utils::logIds(iter->node) << "\n"
            << "-> invalid node id: '" << iter->node->id() << "'";
        return nullptr;
    }
    if (iter->node->id() != nodeId)
    {
        gtError().verbose()
            << gt::log::nospace
            << utils::logIds(this)
            << "::findNode(" << nodeId << ")\n"
            << "-> found node: " << utils::logIds(iter->node) << "\n"
            << "-> invalid node id: '" << iter->node->id() << "'";
        assert(iter->node->id() == nodeId);
    }
    if (iter->node->parent() != this)
    {
        gtError().verbose()
            << gt::log::nospace
            << utils::logIds(this)
            << "::findNode(" << nodeId << ")\n"
            << "-> found node: " << utils::logIds(iter->node) << "\n"
            << "-> invalid parent: " << utils::logIds(static_cast<Node*>(accessGraph(*iter->node)));
        assert(iter->node->id() == nodeId);
    }

    return iter->node;
}

Node const*
Graph::findNode(NodeId nodeId) const
{
    return const_cast<Graph*>(this)->findNode(nodeId);
}

Node*
Graph::findNodeByUuid(NodeUuid const& uuid)
{
    return qobject_cast<Node*>(getObjectByUuid(uuid));
}

Node const*
Graph::findNodeByUuid(NodeUuid const& uuid) const
{
    return const_cast<Graph*>(this)->findNodeByUuid(uuid);
}

Connection*
Graph::findConnection(ConnectionId conId)
{
    auto const& childs = connectionGroup().children();
    for (auto* child : childs)
    {
        if (auto* con = qobject_cast<Connection*>(child))
        {
            if (con->connectionId() == conId) return con;
        }
    }
    return nullptr;
}

Connection const*
Graph::findConnection(ConnectionId conId) const
{
    return const_cast<Graph*>(this)->findConnection(conId);
}

QVector<ConnectionId>
Graph::findConnections(NodeId nodeId, PortType type) const
{
    auto iter = pimpl->local.find(nodeId);
    if (iter == pimpl->local.end()) return {};

    QVector<ConnectionId> connections;
    if (type == PortType::NoType)
    {
        std::copy(iter->iterateConnections().begin(),
                  iter->iterateConnections().end(),
                  std::back_inserter(connections));
    }
    else
    {
        std::copy(iter->iterateConnections(type).begin(),
                  iter->iterateConnections(type).end(),
                  std::back_inserter(connections));
    }

    return connections;
}

QVector<ConnectionId>
Graph::findConnections(NodeId nodeId, PortId portId) const
{
    auto iter = pimpl->local.find(nodeId);
    if (iter == pimpl->local.end()) return {};

    QVector<ConnectionId> connections;
    std::copy(iter->iterateConnections(portId).begin(),
              iter->iterateConnections(portId).end(),
              std::back_inserter(connections));
    return connections;
}

QVector<NodeId>
Graph::uniqueTargetNodes(QVector<ConnectionId> const& connections, PortType type)
{
    QVector<NodeId> nodes;
    for (ConnectionId conId : connections)
    {
        if (type == PortType::In) conId = conId.reversed();
        if (!nodes.contains(conId.inNodeId)) nodes.push_back(conId.inNodeId);
    }
    return nodes;
}

QVector<NodeId>
Graph::findConnectedNodes(NodeId nodeId, PortType type) const
{
    QVector<NodeId> nodes;
    auto iter = pimpl->local.iterateUniqueNodes(nodeId, type);
    std::copy(iter.begin(), iter.end(), std::back_inserter(nodes));
    return nodes;
}

QVector<NodeId>
Graph::findConnectedNodes(NodeId nodeId, PortId portId) const
{
    QVector<NodeId> nodes;
    auto iter = pimpl->local.iterateUniqueNodes(nodeId, portId);
    std::copy(iter.begin(), iter.end(), std::back_inserter(nodes));
    return nodes;
}

QList<Graph*>
Graph::graphNodes()
{
    return findDirectChildren<Graph*>();
}

QList<Graph const*>
Graph::graphNodes() const
{
    return gt::container_const_cast(
        const_cast<Graph*>(this)->graphNodes()
    );
}

void
Graph::clearGraph()
{
    auto* guiData = findDirectChild<GuiData*>();
    if (guiData) guiData->clearData();

    // connections should be removed automatically
    qDeleteAll(nodes());
}

PortId
Graph::portId(NodeId nodeId, PortType type, PortIndex portIdx) const
{
    auto const makeError = [nodeId](){
        return tr("Failed to get port id for node %1!").arg(nodeId);
    };

    auto* node = findNode(nodeId);
    if (!node)
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << tr("(node not found)");
        return invalid<PortId>();
    }

    PortId portId = node->portId(type, portIdx);
    if (portId == invalid<PortId>())
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << tr("(port idx %1 of type %2 out of bounds)")
                           .arg(portIdx).arg(toString(type));
        return invalid<PortId>();
    }

    return portId;
}

NodeId
Graph::nodeId(NodeUuid const& nodeUuid) const
{
    Node const* node = globalConnectionModel().node(nodeUuid);
    if (!node || Graph::accessGraph(*node) != this) return NodeId{};

    return node->id();
}

NodeUuid
Graph::nodeUuid(NodeId nodeId) const
{
    Node const* node = connectionModel().node(nodeId);
    if (!node || Graph::accessGraph(*node) != this) return NodeUuid{};

    return node->uuid();
}

ConnectionId
Graph::connectionId(NodeId outNodeId, PortIndex outPortIdx, NodeId inNodeId, PortIndex inPortIdx) const
{
    auto const makeError = [](){
        return tr("Failed to create connection id!");
    };
    auto const nodeNotFound = [](NodeId id){
        return tr("(node '%1' not found)").arg(id);
    };
    auto const portOutOfBounds = [](NodeId id, PortIndex idx){
        return tr("(port %1 of node %2 is out of bounds)").arg(idx, id);
    };

    auto* outNode = findNode(outNodeId);
    auto* inNode  = findNode(inNodeId);
    if (!outNode || !inNode)
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << (!outNode ? nodeNotFound(outNodeId) :
                                   nodeNotFound(inNodeId));
        return invalid<ConnectionId>();
    }

    auto outPort = outNode->portId(PortType::Out, outPortIdx);
    auto inPort  = inNode->portId(PortType::In, inPortIdx);
    if (outPort == invalid<PortId>() || inPort == invalid<PortId>())
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << (outPort == invalid<PortId>() ?
                                   portOutOfBounds(outNodeId, outPortIdx) :
                                   portOutOfBounds(inNodeId, inPortIdx));
        return invalid<ConnectionId>();
    }

    return { outNode->id(), outPort, inNode->id(), inPort };
}

ConnectionId
Graph::connectionId(ConnectionUuid const& conUuid) const
{
    auto const makeError = [&conUuid](){
        return tr("Failed to convert connection uuid (%1)!")
                   .arg(toString(conUuid));
    };
    auto const nodeNotFound = [](NodeUuid uuid){
        return tr("(node '%1' not found)").arg(uuid);
    };

    auto* outNode = findNodeByUuid(conUuid.outNodeId);
    auto* inNode  = findNodeByUuid(conUuid.inNodeId);
    if (!outNode || !inNode)
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << (!outNode ? nodeNotFound(conUuid.outNodeId) :
                                   nodeNotFound(conUuid.inNodeId));
        return invalid<ConnectionId>();
    }
    if (outNode->parent() != this || inNode->parent() != this)
    {
        gtWarning() << utils::logIds(*this) << makeError()
                    << tr("(nodes do not belong to this graph '%3')")
                           .arg(relativeNodePath(*this));
        return invalid<ConnectionId>();
    }

    return { outNode->id(), conUuid.outPort, inNode->id(), conUuid.inPort };
}

ConnectionUuid
Graph::connectionUuid(ConnectionId conId) const
{
    auto const makeError = [conId](){
        return tr("Failed to convert connection id (%1)!")
                   .arg(toString(conId));
    };
    auto const nodeNotFound = [](NodeId id){
        return tr("(node %1 not found)").arg(id);
    };

    auto* outNode = findNode(conId.outNodeId);
    auto* inNode  = findNode(conId.inNodeId);
    if (!outNode || !inNode)
    {
        gtWarning( )<< utils::logIds(*this) << makeError()
                    << (!outNode ? nodeNotFound(conId.outNodeId) :
                                   nodeNotFound(conId.inNodeId));
        return invalid<ConnectionUuid>();
    }

    return { outNode->uuid(), conId.outPort, inNode->uuid(), conId.inPort };
}

bool
Graph::canAppendConnection(ConnectionId conId) const
{
    return Impl::canAppendConnection(*this, conId);
}

Node*
Graph::appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy)
{
    if (!appendNode(node.get(), policy)) return nullptr;

    return node.release();
}

bool
Graph::appendNode(Node* node, NodeIdPolicy policy)
{
    if (!node) return false;

    auto makeError = [n = node, this](){
        return utils::logId(*this) + QChar{' '} +
               tr("Failed to append node '%1' to intelli graph '%2'!")
                   .arg(n->objectName(), objectName());
    };

    // check if node exists and update node id if necessary
    if (!Impl::updateNodeId(*this, *node, policy))
    {
        gtWarning() << makeError()
                    << tr("(node already exists)");
        return false;
    }

    // check if node can be appended
    if (!Impl::canAppendNode(*this, *node, makeError)) return {};

    // append node to hierarchy
    if (!appendChild(node))
    {
        gtWarning() << makeError();
        return false;
    }

    node->updateObjectName();

    // deprecation notice
    if (node->nodeFlags() & NodeFlag::Deprecated &&
        gt::log::Logger::instance().verbosity() >= gt::log::Verbosity::Medium)
    {
        gtLogOnce(Warning)
            << utils::logIds(*this)
            << tr("Node '%1' is deprecated and may be removed in "
                  "a future release of the associated module!")
                   .arg(node->metaObject()->className());
    }

    // append nodes of subgraph
    if (auto* graph = qobject_cast<Graph*>(node))
    {
        graph->updateGlobalConnectionModel(pimpl->global);

        // init input output providers of sub graph
        graph->initInputOutputProviders();

        mergeUserVariables(*graph);
    }

    gtDebug() << utils::logIds(*this) << "appending node" << utils::logIds(node) << "into" << (void*)pimpl->global.get();

    // register node in local model
    pimpl->local.insert(node->id(), node);

    // register node in global model if not present already (avoid overwrite)
    NodeUuid const& nodeUuid = node->uuid();
    if (!pimpl->global->contains(nodeUuid)) pimpl->global->insert(nodeUuid, node);
    else
    {
        gtDebug() << utils::logIds(*this) << "NODE ALREAY APPENDED TO GLOBAL MODEL" << node->uuid();
    }

    // setup connections
    connect(node, &Node::portChanged,
            this, Impl::PortChanged(this, node),
            Qt::DirectConnection);

    connect(node, &Node::portInserted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortInserted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node, &Node::portAboutToBeDeleted,
            this, Impl::PortDeleted(this, node),
            Qt::DirectConnection);

    connect(node, &Node::portDeleted,
            this, [this, nodeId = node->id()](PortType type, PortIndex idx){
        emit nodePortDeleted(nodeId, type, idx);
    }, Qt::DirectConnection);

    connect(node, &Node::nodeAboutToBeDeleted,
            this, Impl::NodeDeleted(this),
            Qt::DirectConnection);

//    if (pimpl->forwardInvalidation)
//    {
//        connect(node, &Node::triggerNodeEvaluation,
//                this, &Graph::childNodeInvalidated,
//                Qt::DirectConnection);
//    }

    // notify
    emit nodeAppended(node);

    return true;
}

bool
Graph::appendConnection(ConnectionId conId)
{
    return appendConnection(std::make_unique<Connection>(conId));
}

Connection*
Graph::appendConnection(std::unique_ptr<Connection> connection)
{
    if (!appendConnection(connection.get())) return nullptr;

    return connection.release();
}

bool
Graph::appendConnection(Connection* connection)
{
    if (!connection) return false;

    auto conId = connection->connectionId();

    auto makeError = [conId, this](){
        return gt::quoted(relativeNodePath(*this), "<", "> ") +
               tr("Failed to append connection '%1'!")
                   .arg(toString(conId));
    };

    // check if connection can be appended
    constexpr bool silent = false;
    if (!Impl::canAppendConnection(*this, conId, makeError, silent)) return {};

    // append connection to hierarchy
    if (!connectionGroup().appendChild(connection))
    {
        gtWarning() << makeError();
        return false;
    }

    connection->updateObjectName();

    // append connection to model
    auto targetNode = pimpl->local.find(conId.inNodeId);
    auto sourceNode = pimpl->local.find(conId.outNodeId);
    assert(targetNode != pimpl->local.end());
    assert(sourceNode != pimpl->local.end());

    auto inConnection  = ConnectionDetail<NodeId>::fromConnection(conId.reversed());
    auto outConnection = ConnectionDetail<NodeId>::fromConnection(conId);

    targetNode->predecessors.append(inConnection);
    sourceNode->successors.append(outConnection);

    // setup connections
    connect(connection, &QObject::destroyed,
            this, Impl::ConnectionDeleted(this, conId),
            Qt::DirectConnection);

    // append to global connection model
    appendGlobalConnection(connection, conId, *targetNode->node);

    // notify
    emit connectionAppended(conId);

    assert(targetNode->iterateConnections(conId.inPort).size() == 1);

    emit targetNode->node->portConnected(conId.inPort);
    emit sourceNode->node->portConnected(conId.outPort);

    return true;
}

void
Graph::appendGlobalConnection(Connection* guard, ConnectionId conId, Node& targetNode)
{
    auto* root = rootGraph();
    assert(root);

    auto conUuid = connectionUuid(conId);
    assert(conUuid.isValid());

    appendGlobalConnection(guard, conUuid);

//    // forwards inputs of graph node to subgraph
//    if (auto* graph = qobject_cast<Graph*>(&targetNode))
//    if (graph->metaObject()->className() == GT_CLASSNAME(Graph))
//    {
//        auto* inputProvider = graph->inputProvider();
//        assert(inputProvider);

//        conUuid.inNodeId = inputProvider->uuid();
//        conUuid.inPort   = GroupInputProvider::virtualPortId(conId.inPort);

//        appendGlobalConnection(guard, conUuid);
//    }

//    // forwards outputs of subgraph to graph node
//    if (auto* output = qobject_cast<GroupOutputProvider*>(&targetNode))
//    if (output->parent()->metaObject()->className() == GT_CLASSNAME(Graph))
//    {
//        NodeUuid const& graphUuid = uuid();

//        // graph is being restored (memento diff)
//        if (!pimpl->global->contains(graphUuid))
//        {
//            assert(isBeingModified());
//            pimpl->global->insert(graphUuid, this);
//        }

//        conUuid.outNodeId = output->uuid();
//        conUuid.outPort   = GroupOutputProvider::virtualPortId(conUuid.inPort);
//        conUuid.inNodeId  = graphUuid;
//        conUuid.inPort    = conUuid.outPort;

//        appendGlobalConnection(nullptr, std::move(conUuid));
//    }
}

void
Graph::appendGlobalConnection(Connection* guard, ConnectionUuid conUuid)
{
    assert(conUuid.isValid());

    auto globalTargetNode = pimpl->global->find(conUuid.inNodeId);
    auto globalSourceNode = pimpl->global->find(conUuid.outNodeId);

    if (globalTargetNode == pimpl->global->end())
    {
        gtError() << utils::logIds(*this)
                  <<tr("Failed to append connection, target node not found! "
                        "(Connection: %1)").arg(conUuid.inNodeId);
        return;
    }
    if (globalSourceNode == pimpl->global->end())
    {
        gtError() << utils::logIds(*this)
                  << tr("Failed to append connection, source node not found! "
                        "(Connection: %1)").arg(conUuid.outNodeId);
        return;
    }

    auto inConnection  = ConnectionDetail<NodeUuid>::fromConnection(conUuid.reversed());
    auto outConnection = ConnectionDetail<NodeUuid>::fromConnection(conUuid);

    globalTargetNode->predecessors.append(inConnection);
    globalSourceNode->successors.append(outConnection);

    if (guard)
    {
        // setup connections
        connect(guard, &QObject::destroyed,
                this, Impl::GlobalConnectionDeleted(this, conUuid),
                Qt::DirectConnection);
    }

    emit globalConnectionAppended(conUuid);
}

QVector<NodeId>
Graph::appendObjects(std::vector<std::unique_ptr<Node>>& nodes,
                     std::vector<std::unique_ptr<Connection>>& connections)
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    QVector<NodeId> nodeIds;

    for (auto& obj : nodes)
    {
        if (!obj) return nodeIds;

        auto oldId = obj->id();

        auto* node = appendNode(std::move(obj));
        if (!node) return nodeIds;

        auto newId = node->id();

        nodeIds.push_back(newId);

        if (oldId == newId) continue;

        // update connections
        for (auto& con : connections)
        {
            if (con->inNodeId() == oldId)
            {
                con->setInNodeId(newId);
            }
            else if (con->outNodeId() == oldId)
            {
                con->setOutNodeId(newId);
            }
        }
    }

    for (auto& obj : connections)
    {
        // TODO: signal to reciever that the method failed
        if (!appendConnection(std::move(obj))) return nodeIds;
    }

    return nodeIds;
}

bool
Graph::deleteNode(NodeId nodeId)
{
    if (auto* node = findNode(nodeId))
    {
        gtInfo().verbose()
            << utils::logIds(*this)
            << tr("Deleting node:") << node->objectName();
        delete node;
        return true;
    }
    return false;
}

bool
Graph::deleteConnection(ConnectionId connectionId)
{
    if (auto* connection = findConnection(connectionId))
    {
        gtInfo().verbose()
            << utils::logIds(*this)
            << tr("Deleting connection:") << connectionId;
        delete connection;
        return true;
    }
    return false;
}

bool
Graph::moveNode(NodeId nodeId, Graph& targetGraph, NodeIdPolicy policy)
{
    Node* node = findNode(nodeId);
    return node && moveNode(*node, targetGraph, policy);
}

bool
Graph::moveNode(Node& node, Graph& targetGraph, NodeIdPolicy policy)
{
    Modification cmd = modify();
    Q_UNUSED(cmd);

    assert(node.parent() == this);

    // restore ownership on failure
    auto restoreOnFailure = gt::finally([&node, this](){
        ((QObject*)&node)->setParent(this);
    });

    // update connection model
    Impl::NodeDeleted(this)(node.id());

    // models need to be reset once moving finished to avoid inconsistent states
    if (qobject_cast<Graph*>(&node))
    {
        pimpl->resetAfterModification = true;
        targetGraph.pimpl->resetAfterModification = true;
    }

    node.disconnect(this);
    node.disconnectFromParent();
    ((QObject*)&node)->setParent(nullptr);

    if (!targetGraph.appendNode(&node, policy)) return false;

    restoreOnFailure.clear();
    return true;
}

namespace
{

template<typename NodeList>
bool moveNodesHelper(Graph& sourceGraph,
                     NodeList const& nodes,
                     Graph& targetGraph,
                     NodeIdPolicy policy)
{
    auto changeCmd = sourceGraph.modify();
    auto changeTargetCmd = targetGraph.modify();
    Q_UNUSED(changeCmd);
    Q_UNUSED(changeTargetCmd);

    // NOTE: Cannot use connectionModel.iterate-functions as iterators may get
    // invalidated if a node is removed!
    return std::all_of(nodes.begin(), nodes.end(),
                       [&sourceGraph, &targetGraph, policy](auto node){
        return sourceGraph.moveNode(get_node_id<NodeId>{}(node), targetGraph, policy);
    });
}

template <typename NodeList>
bool moveNodesAndConnectionsHelper(Graph& sourceGraph,
                                   NodeList const& nodes,
                                   Graph& targetGraph,
                                   NodeIdPolicy policy = NodeIdPolicy::Update)
{
    auto changeCmd = sourceGraph.modify();
    auto changeTargetCmd = targetGraph.modify();
    Q_UNUSED(changeCmd);
    Q_UNUSED(changeTargetCmd);

    auto const& conModel = sourceGraph.connectionModel();

    QVector<ConnectionUuid> connectionsToMove;
    // find internal connections
    for (auto node : nodes)
    {
        auto iter = conModel.iterateConnections(get_node_id<NodeId>{}(node), PortType::Out);
        for (ConnectionId conId : iter)
        {
            if (containsNodeId(conId.inNodeId, nodes))
            {
                connectionsToMove.push_back(sourceGraph.connectionUuid(conId));
            }
        }
    }

    if (!::moveNodesHelper(sourceGraph, nodes, targetGraph, policy)) return false;

    // reinstantiate internal connections
    bool success = std::all_of(connectionsToMove.begin(),
                               connectionsToMove.end(),
                               [&targetGraph](auto const& conUuid){
        return targetGraph.appendConnection(targetGraph.connectionId(conUuid));
    });

    return success;
}


} // namespace

bool
Graph::moveNodesAndConnections(View<Node const*> nodes,
                               Graph& targetGraph,
                               NodeIdPolicy policy)
{
    return ::moveNodesAndConnectionsHelper(*this, nodes, targetGraph, policy);
}

bool
Graph::moveNodesAndConnections(View<NodeId> nodes,
                               Graph& targetGraph,
                               NodeIdPolicy policy)
{
    return ::moveNodesAndConnectionsHelper(*this, nodes, targetGraph, policy);
}

bool
Graph::moveNodesAndConnections(QList<Node const*> const& nodes,
                               Graph& targetGraph,
                               NodeIdPolicy policy)
{
    return ::moveNodesAndConnectionsHelper(*this, nodes, targetGraph, policy);
}

bool
Graph::moveNodes(View<Node const*> nodes,
                 Graph& targetGraph,
                 NodeIdPolicy policy)
{
    return ::moveNodesHelper(*this, nodes, targetGraph, policy);
}

bool
Graph::moveNodes(View<NodeId> nodes,
                 Graph& targetGraph,
                 NodeIdPolicy policy)
{
    return ::moveNodesHelper(*this, nodes, targetGraph, policy);
}

bool
Graph::moveNodes(QList<Node const*> const& nodes,
                 Graph& targetGraph,
                 NodeIdPolicy policy)
{
    return ::moveNodesAndConnectionsHelper(*this, nodes, targetGraph, policy);
}

void
Graph::onObjectDataMerged()
{
    DynamicNode::onObjectDataMerged();

    restoreNodesAndConnections();

    if (pimpl->resetAfterMerge)
    {
        pimpl->resetAfterMerge = false;

        // TODO: this should be fixed with #1370 (GTlab Core)
        // issue arises because provider nodes are added in the constructor, their
        // uuid changes once the initial memento is applied. Thus, the connection
        // model is outdated
        gtTrace().verbose()
            << utils::logIds(*this)
            << tr("Resetting global connection model!");
        resetGlobalConnectionModel();
    }
}

void
Graph::restoreNode(Node* node)
{
    assert(node);
    if (findNode(node->id()))
    {
#ifndef NDEBUG
        if (auto* subgraph = qobject_cast<Graph*>(node))
        {
            assert(subgraph->pimpl->global == pimpl->global);
        }
#endif
        // TODO: this should be fixed with #1370 (GTlab Core)
        if (pimpl->global->node(node->uuid()) != node)
        {
            pimpl->resetAfterMerge = true;
            gtError().verbose()
                << utils::logIds(*this)
                << tr("Node '%1' changed uuid, need to reset model!")
                       .arg(relativeNodePath(*node));
        }
        return;
    }

    std::unique_ptr<Node> ptr{node};
    static_cast<QObject*>(node)->setParent(nullptr);

    appendNode(std::move(ptr));
}

void
Graph::restoreConnection(Connection* connection)
{
    assert(connection);
    auto conId = connection->connectionId();

    auto cons = pimpl->local.iterateConnections(conId.inNodeId, PortType::In);
    if (std::find(cons.begin(), cons.end(), conId) != cons.end())
    {
        cons = pimpl->local.iterateConnections(conId.outNodeId, PortType::Out);
        assert(std::find(cons.begin(), cons.end(), conId) != cons.end());
        return;
    }

    cons = pimpl->local.iterateConnections(conId.outNodeId, PortType::Out);
    assert(std::find(cons.begin(), cons.end(), conId) == cons.end());

    std::unique_ptr<Connection> ptr{connection};
    ptr->setParent(nullptr);

    if (!appendConnection(connection))
    {
        bool success = false;

        // attempt to restore dummy connection
        if (DummyNode* inNode = qobject_cast<DummyNode*>(findNode(connection->inNodeId())))
        {
            if (!inNode->port(connection->inPort()))
            {
                inNode->addInPort(PortInfo::customId(connection->inPort(), typeId<InvalidData>()));
                success = true;
            }
        }
        if (DummyNode* outNode = qobject_cast<DummyNode*>(findNode(connection->outNodeId())))
        {
            if (!outNode->port(connection->outPort()))
            {
                outNode->addOutPort(PortInfo::customId(connection->outPort(), typeId<InvalidData>()));
                success = true;
            }
        }

        if (success) appendConnection(std::move(ptr));
    }
    ptr.release();
}

void
Graph::restoreConnections()
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    auto const& connections = this->connections();

    for (auto* connection : connections)
    {
        if (!findNode(connection->inNodeId()) ||
            !findNode(connection->outNodeId())) continue;

        restoreConnection(connection);
    }
}

void
Graph::restoreNodesAndConnections()
{
    auto cmd = modify();
    Q_UNUSED(cmd);

    auto const& objects = this->findDirectChildren<GtObject*>();
    auto const& connections = this->connections();

    for (GtObject* object : objects)
    {
        if (auto* node = qobject_cast<Node*>(object))
        {
            restoreNode(node);
        }
        else if (object->isDummy())
        {
            // check if dummy node is already restored
            auto const& dummies = this->findDirectChildren<DummyNode const*>();

            auto const isRestored = [uuid = object->uuid()](DummyNode const* d){
                return d->linkedUuid() == uuid;
            };
            if (std::any_of(dummies.begin(), dummies.end(), isRestored))
            {
#ifndef GT_INTELLI_DEBUG_NODE_PROPERTIES
                object->setUserHidden(true);
#endif
                continue;
            }

            // add dummy node
            auto dummy = std::make_unique<DummyNode>();
            dummy->setDummyObject(*object);
            appendNode(std::move(dummy));

#ifndef GT_INTELLI_DEBUG_NODE_PROPERTIES
            object->setUserHidden(true);
#endif
        }
    }

    for (auto* connection : connections)
    {
        restoreConnection(connection);
    }
}

void
Graph::initInputOutputProviders()
{
    auto* input = inputProvider();
    auto* output = outputProvider();
    assert(input);
    assert(output);

    input->setUserHidden(false);
    output->setUserHidden(false);
}

void
Graph::resetGlobalConnectionModel()
{
    Modification cmd = modify();
    Q_UNUSED(cmd);

    pimpl->global->clear();
    pimpl->repopulateGlobalConnectionModel(*this);
}

void
Graph::eval()
{
    auto* interface = exec::nodeDataInterface(*this);
    if (qobject_cast<GraphExecutionModel*>(interface))
    {
        for (auto& port : ports(PortType::Out))
        {
            if (port.visible)
            {
                setNodeData(port.id(), nodeData(port.id() + PortId(2)));
            }
        }
        return;
    }

    auto makeError = [this](){
        return gt::quoted(relativeNodePath(*this), "[", "] ") +
               tr("evaluation failed!");
    };

    // setup
    GraphInputProvider* inputNode = inputProvider();
    GraphOutputProvider* outputNode = outputProvider();

    if (!inputNode || !outputNode)
    {
        gtError() << makeError() << tr("input/ouput providers not found!");
        return evalFailed();
    }

    auto* dataModel = qobject_cast<GraphDataModel*>(exec::nodeDataInterface(*this));
    if (!dataModel)
    {
        gtError() << makeError() << tr("data model not found!");
        return evalFailed();
    }

    // set input data
    for (NodePort const& port : ports(PortType::In))
    {
        // TODO: remove invisible ports
        if (!port.visible) continue;

        if (!inputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in input provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        if (!dataModel->setNodeData(inputNode->uuid(), port.id(), nodeData(port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set input data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }

    GtEventLoop loop{std::chrono::seconds{10}};

    // evaluate branch
    GraphExecutor executor{*this, *dataModel};

    loop.connectSuccess(&executor, &GraphExecutor::targetNodesEvaluated);
    loop.connectAbort(this, &Graph::graphAboutToBeDeleted);
    // TODO: evaluate branch only

    auto future = executor.evaluateGraph();
    // TODO: cannot block main thread here

    if (loop.exec() != GtEventLoop::Success)
    {
        return evalFailed();
    }

    // set output data
    for (NodePort const& port : ports(PortType::Out))
    {
        // TODO: remove invisible ports
        if (!port.visible) continue;

        if (!outputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in output provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        if (!setNodeData(port.id(), dataModel->nodeData(outputNode->uuid(), port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set output data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }
}

Graph::Modification
Graph::modify()
{
    emitBeginModification();
    return gt::finally(EndModificationFunctor{this});
}

bool
Graph::isBeingModified() const
{
    return pimpl->modificationCount > 0;
}

void
Graph::emitBeginModification()
{
    assert(pimpl->modificationCount >= 0);

    pimpl->modificationCount++;
    if (pimpl->modificationCount == 1) emit beginModification(QPrivateSignal());
}

void
Graph::emitEndModification()
{
    pimpl->modificationCount--;
    assert(pimpl->modificationCount >= 0);

    if (pimpl->modificationCount == 0)
    {
        if (pimpl->resetAfterModification)
        {
            // do not trigger this function twice
            pimpl->modificationCount++;
            auto cleanup = gt::finally([this](){ pimpl->modificationCount--; });
            Q_UNUSED(cleanup);

            pimpl->resetAfterModification = false;
            rootGraph()->resetGlobalConnectionModel();
        }

        emit endModification(QPrivateSignal());
    }
}

void
Graph::updateGlobalConnectionModel(std::shared_ptr<GlobalConnectionModel> const& ptr)
{
    // merge connection models
    if (ptr != pimpl->global)
    {
        ptr->insert(*pimpl->global);
    }

    pimpl->global = ptr;

    // apply recursively
    for (Graph* subgraph : graphNodes())
    {
        subgraph->updateGlobalConnectionModel(ptr);
    }
}

void
Graph::synchronizePorts(AbstractGraphProvider& provider)
{
    connect(this, &Node::portInserted,
            &provider, std::bind(Impl::onPortInserted,
                                 this,
                                 &provider,
                                 std::placeholders::_1,   // type
                                 std::placeholders::_2),  // idx
            Qt::DirectConnection);
    connect(this, &Node::portChanged,
            &provider, std::bind(Impl::onPortChanged,
                                 this,
                                 &provider,
                                 std::placeholders::_1),  // portId
            Qt::DirectConnection);
    connect(this, &Node::portDeleted,
            &provider, std::bind(Impl::onPortDeleted,
                                 this,
                                 &provider,
                                 std::placeholders::_1,   // type
                                 std::placeholders::_2),  // idx
            Qt::DirectConnection);
}

void
Graph::onPortInserted(PortType type, PortIndex idx)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    if (type == PortType::In)
    {
        Impl::onPortInserted(this, inputProvider(), type, idx);
    }
    else
    {
        Impl::onPortInserted(this, outputProvider(), type, idx);
    }
}

void
Graph::onPortChanged(PortId portId)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    PortType type = portType(portId);
    if (type == PortType::In)
    {
        Impl::onPortChanged(this, inputProvider(), portId);
    }
    else
    {
        Impl::onPortChanged(this, outputProvider(), portId);
    }
}

void
Graph::onPortDeleted(PortType type, PortIndex idx)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    if (type == PortType::In)
    {
        Impl::onPortDeleted(this, inputProvider(), type, idx);
    }
    else
    {
        Impl::onPortDeleted(this, outputProvider(), type, idx);
    }
}

void
Graph::mergeUserVariables(Graph& other)
{
    GraphUserVariables* otherUV = other.findDirectChild<GraphUserVariables*>();
    GraphUserVariables* thisUV = this->findDirectChild<GraphUserVariables*>();

    if (!otherUV || !thisUV) return;

    thisUV->mergeWith(*otherUV);
}

GtMdiItem*
intelli::show(Graph& graph)
{
    return gtMdiLauncher->open(QStringLiteral("intelli::GraphEditor"), &graph);
}

GtMdiItem*
intelli::show(std::unique_ptr<Graph> graph)
{
    if (!graph) return nullptr;

    auto* item = show(*graph);
    if (!item)
    {
        gtWarning() << QObject::tr("Failed to open Graph Editor for intelli graph")
                    << graph->caption();
        return nullptr;
    }

    assert(item->parent() == item->widget());
    assert(item->parent() != nullptr);

    if (item->appendChild(graph.get())) graph.release();

    return item;
}

bool isAcyclicHelper(Graph const& graph,
                     Node const& node,
                     QVector<NodeId>& visited,
                     QVector<NodeId>& pending)
{
    if (pending.contains(node.id()))
    {
        return false;
    }
    if (visited.contains(node.id()))
    {
        return true;
    }

    pending.push_back(node.id());

    auto conData = graph.connectionModel().find(node.id());
    assert(conData != graph.connectionModel().end());

    for (auto conId : conData->iterateConnections(PortType::Out))
    {
        auto* tmp = graph.findNode(conId.inNodeId);
        if (!tmp)
        {
            gtError() << QObject::tr("Failed to check if graph '%1' is acyclic, node %2 not found!")
                             .arg(graph.objectName(), conId.inNodeId);
            return false;
        }
        if (!isAcyclicHelper(graph, *tmp, visited, pending))
        {
            return false;
        }
    }

    visited.push_back(node.id());
    pending.removeOne(node.id());
    return true;
}

QVector<NodeId>
intelli::cyclicNodes(Graph const& graph)
{
    auto const& nodes = graph.nodes();

    QVector<NodeId> visited;
    QVector<NodeId> pending;

    for (auto* node : nodes)
    {
        if (!isAcyclicHelper(graph, *node, visited, pending))
        {
            return pending;
        }
    }

    return pending;
}

bool
intelli::isAcyclic(Graph const& graph)
{
    return cyclicNodes(graph).empty();
}

namespace
{

template<typename NodeId_t>
inline QString
printableCaption(Node const* node, NodeId_t id)
{
    QString idStr = QStringLiteral("%1").arg(id);
    idStr.remove('{');
    idStr.remove('}');
    idStr.remove('-');

    if (qobject_cast<GraphInputProvider const*>(node))
    {
        return QStringLiteral("id_%1((I))").arg(idStr);
    }
    if (qobject_cast<GraphOutputProvider const*>(node))
    {
        return QStringLiteral("id_%1((O))").arg(idStr);
    }

    QString caption = node ? gt::brackets(node->caption()) : "{NULL}";
    caption.remove(']');
    caption.replace('[', '_');
    caption.replace(' ', '_');
    return QStringLiteral("id_%1%2").arg(idStr, caption);
}

template<typename Model>
QString
debugGraphHelper(Model const& data)
{
    QString debugString;

    auto begin = data.keyValueBegin();
    auto end   = data.keyValueEnd();
    for (auto iter = begin; iter != end; ++iter)
    {
        auto& conData = iter->second;

        QString const& caption = printableCaption(conData.node, iter->first);
        debugString += QStringLiteral("\t") + caption + QStringLiteral("\n");

        for (auto& con : conData.successors)
        {
            auto* otherNode = data.node(con.node);

            QString text = QStringLiteral("\t") + caption +
                           QStringLiteral(" --") + QString::number(con.sourcePort) +
                           QStringLiteral(" : ") + QString::number(con.port) +
                           QStringLiteral("--> ") + printableCaption(otherNode, con.node) +
                           QStringLiteral("\n");
            if (debugString.contains(text)) continue;

            debugString += text;
        }

        for (auto& con : conData.predecessors)
        {
            auto* otherNode = data.node(con.node);

            QString text = QStringLiteral("\t") + printableCaption(otherNode, con.node) +
                           QStringLiteral(" --") + QString::number(con.port) +
                           QStringLiteral(" : ") + QString::number(con.sourcePort) +
                           QStringLiteral("--> ") + caption +
                           QStringLiteral("\n");
            if (debugString.contains(text)) continue;

            debugString += text;
        }
    }

    return debugString;
}

} // namespace

void
intelli::debug(GlobalConnectionModel const& model)
{
    QString text = QStringLiteral("flowchart LR\n");
    text += ::debugGraphHelper(model);

    gtInfo().nospace() << text;
}

void
intelli::debug(ConnectionModel const& model)
{
    QString text = QStringLiteral("flowchart LR\n");
    text += ::debugGraphHelper(model);

    gtInfo().nospace() << text;
}

void
intelli::debug(Graph const& graph)
{
    QString text = QStringLiteral("flowchart LR\n");
    text += ::debugGraphHelper(graph.globalConnectionModel());

    gtInfo().nospace() << "Debugging graph...\n\"\n" << text << "\"";
}
