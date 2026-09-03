/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPH_IMPL_H
#define GT_INTELLI_GRAPH_IMPL_H

#include <intelli/graph.h>
#include <intelli/connection.h>
#include <intelli/nodedatafactory.h>
#include <intelli/node/abstractgroupprovider.h>
#include <intelli/private/utils.h>

#include <gt_logging.h>


namespace intelli
{

/// NOTE: starting id may not be enforced in old GTlab projects yet
/// (not guranteed to be free)
constexpr static NodeId s_startingNodeId{8};

/// Helper struct to "hide" implementation details and template functions
struct Graph::Impl
{
    /// local connection graph
    ConnectionModel local;
    /// shred global connection graph
    std::shared_ptr<GlobalConnectionModel> global = std::make_shared<GlobalConnectionModel>();
    /// indicator if the connection model is currently beeing modified
    int modificationCount = 0;
    /// flag indicating that the connection model should be reset once
    /// the graph is no longer being modified
    bool resetAfterModification = false;

    bool resetAfterMerge = false;

    bool forwardInvalidation = false;

    template <typename MakeError = QString(*)()>
    static inline bool
    canAppendConnection(Graph const& graph,
                        ConnectionId conId,
                        MakeError const& makeError = {},
                        bool silent = true)
    {
        if (!conId.isValid())
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(invalid connection)");
            }
            return false;
        }

        // check if nodes differ
        if (conId.inNodeId == conId.outNodeId)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node and out-node are qeual)");
            }
            return false;
        }

        // connection may already exist
        if (graph.findConnection(conId))
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection already exists)");
            }
            return false;
        }

        // check if nodes exist
        auto& conModel = graph.connectionModel();

        auto target = conModel.find(conId.inNodeId);;
        auto source = conModel.find(conId.outNodeId);

        if (target == conModel.end() || source == conModel.end())
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node %1, out-node %2)")
                                   .arg(target == conModel.end() ? "found" : "not found")
                                   .arg(source == conModel.end() ? "found" : "not found");
            }
            return false;
        }

        if (!target->node || !source->node)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node %1, out-node %2)")
                                   .arg(target->node ? "valid" : "null")
                                   .arg(source->node ? "valid" : "null");
            }
            return false;
        }

        assert(target->node->id() == conId.inNodeId &&
               target->node->parent()  == &graph);
        assert(source->node->id() == conId.outNodeId &&
               source->node->parent() == &graph);

        // check if ports to connect exist
        PortInfo* inPort  = target->node->port(conId.inPort);
        PortInfo* outPort = source->node->port(conId.outPort);

        if (!inPort || !outPort)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-port of node '%3' %1, out-port of node '%4' %2)")
                                   .arg(inPort  ? "found" : "not found",
                                        outPort ? "found" : "not found",
                                        relativeNodePath(*target->node),
                                        relativeNodePath(*source->node));
            }
            return false;
        }

        // check if output is connected to input
        if (target->node->portType(inPort->id())  ==
            source->node->portType(outPort->id()))
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(cannot connect ports of same port side)");
            }
            return false;
        }

        // target node should be an input port
        assert(target->node->portType(inPort->id()) == PortType::In);

        // check if types are compatible
        auto& factory = NodeDataFactory::instance();
        if (!factory.canConvert(inPort->typeId, outPort->typeId))
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(cannot connect ports with incompatible types: %1 vs %2")
                                   .arg(outPort->typeId, inPort->typeId);
            }
            return false;
        }

        // check if input port is already connected
        auto connected = !target->iterateConnections(conId.inPort).empty();
        if (connected)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(in-port is already connected)");
            }
            return false;
        }

        return true;
    }

    template<typename MakeError>
    static inline bool
    canAppendNode(Graph const& graph,
                  Node& node,
                  MakeError const& makeError,
                  bool silent = false)
    {
        // check if node is unique
        if (!(node.nodeFlags() & NodeFlag::Unique)) return true;

        auto const& nodes = graph.nodes();
        for (auto const& entry : graph.pimpl->local)
        {
            assert(entry.node);
            if (entry.node->modelName() == node.modelName())
            {
                if (!silent)
                {
                    gtWarning() << makeError()
                                << tr("(node is unique and already exists)");
                }
                return false;
            }
        }
        return true;
    }

    static inline bool
    accumulateDependentNodes(Graph const& graph,
                             QVector<NodeId>& nodes,
                             NodeId nodeId,
                             PortType type)
    {
        auto& conModel = graph.connectionModel();
        auto connections = conModel.iterate(nodeId, type);

        for (auto& con : connections)
        {
            if (nodes.contains(con.node)) continue;
            nodes.append(con.node);
            if (!accumulateDependentNodes(graph, nodes, con.node, type))
            {
                return false;
            }
        }

        return true;
    }

    /// checks and updates the node id of the node depending of the policy specified
    static inline bool
    updateNodeId(Graph const& graph, Node& node, NodeIdPolicy policy)
    {
        auto const nodes = graph.nodes();

        // id may already be used
        QVector<NodeId> ids;
        ids.reserve(nodes.size());
        std::transform(std::begin(nodes), std::end(nodes),
                       std::back_inserter(ids), [](Node const* n){
            return n->id();
        });

        if (node.id() == invalid<NodeId>() || ids.contains(node.id()))
        {
            if (policy != NodeIdPolicy::Update) return false;

            // generate a new one
            size_t maxId = ids.empty() ?
                               s_startingNodeId :
                               std::max(
                                   *std::max_element(
                                       std::begin(ids),
                                       std::end(ids)) + NodeId{1},
                                   s_startingNodeId);
            node.setId(NodeId::fromValue(maxId));
            return node.id().isValid();
        }
        return true;
    }

    /// recursively updates the global connection model of `graph` by inserting
    /// nodes and setting connections
    static inline void
    repopulateGlobalConnectionModel(Graph& graph)
    {
        // disconnect, incase connection was moved
        for (Connection* connection : graph.connections())
        {
            connection->disconnect(&graph);
        }
        // append nodes first
        for (auto& entry : graph.pimpl->local)
        {
            graph.pimpl->global->insert(entry.node->uuid(), entry.node);
        }
        // recurisvely append nodes and connections
        for (Graph* subgraph : graph.graphNodes())
        {
            assert(graph.pimpl->global.get() == subgraph->pimpl->global.get());
            repopulateGlobalConnectionModel(*subgraph);
        }
        // append connections of this graph
        for (auto& entry : graph.pimpl->local)
        {
            for (auto& conId : entry.iterateConnections(PortType::Out))
            {
                Connection* connection = graph.findConnection(conId);
                assert(connection);
                Node* targetNode = graph.findNode(conId.inNodeId);
                assert(targetNode);

                // reconnect
                connect(connection, &QObject::destroyed,
                        &graph, Impl::ConnectionDeleted(&graph, conId),
                        Qt::DirectConnection);

                graph.appendGlobalConnection(connection, conId, *targetNode);
            }
        }
    }

    static void onPortInserted(Node* root,
                               AbstractGraphProvider* provider,
                               PortType type,
                               PortIndex idx,
                               bool invert = false)
    {
        assert(root);
        assert(!qobject_cast<DynamicNode*>(root) ||
               static_cast<DynamicNode*>(root)->isDynamicPort(type, idx));
        assert(provider);

        if (!invert && type != provider->providerType()) return;

        auto const makeError = [root, type, idx](){
            return relativeNodePath(*root) + QStringLiteral(": ") +
                   QObject::tr("Failed to add %3put port (%1/%2)!")
                       .arg(toString(idx),
                            toString(type),
                            type == PortType::In ? "in":"out");
        };

        PortInfo* srcPort = root->port(root->portId(type, idx));
        if (!srcPort)
        {
            gtError() << makeError() << QObject::tr("(Source port not found)");
            return;
        }

        if (!provider)
        {
            gtError() << makeError() << tr("(provider not found)");
            return;
        }

        PortId addedPortId = provider->addPort(*srcPort);
        if (!addedPortId.isValid())
        {
            gtError() << makeError()
                      << QObject::tr("(Adding port to provider failed)");
            return;
        }
        assert(addedPortId == srcPort->id());
    }

    static void onPortChanged(Node* root,
                              AbstractGraphProvider* provider,
                              PortId portId,
                              bool invert = false)
    {
        assert(portId.isValid());
        assert(root);
        assert(provider);

        PortType type = root->portType(portId);
        assert(type != PortType::NoType);
        assert(!qobject_cast<DynamicNode*>(root) ||
               static_cast<DynamicNode*>(root)->isDynamicPort(type, root->portIndex(type, portId)));

        if (!invert && type != provider->providerType()) return;

        auto const makeError = [root, type, portId](){
            return relativeNodePath(*root) + QStringLiteral(": ") +
                   tr("Failed to update %2put port (%1)!")
                       .arg(toString(portId),
                            type == PortType::In ? "in":"out");
        };

        PortInfo* srcPort = root->port(portId);
        if (!srcPort)
        {
            gtError() << makeError() << tr("(Source port not found)");
            return;
        }

        if (!provider)
        {
            gtError() << makeError() << tr("(provider not found)");
            return;
        }

        PortInfo* port = provider->port(srcPort->id());
        if (!port)
        {
            gtError() << makeError()
                      << QObject::tr("(Updating port of provider failed)");
            return;
        }
        port->assign(*srcPort);
        emit provider->portChanged(port->id());
    }

    static void onPortDeleted(Node* root,
                              AbstractGraphProvider* provider,
                              PortType type,
                              PortIndex idx,
                              bool invert = false)
    {
        assert(root);
        assert(!qobject_cast<DynamicNode*>(root) ||
               static_cast<DynamicNode*>(root)->isDynamicPort(type, idx));
        assert(provider);

        if (!invert && type != provider->providerType()) return;

        auto const makeError = [root, type, idx](){
            return relativeNodePath(*root) + QStringLiteral(": ") +
                   tr("Failed to delete %3put port (%1/%2)!")
                       .arg(toString(idx),
                            toString(type),
                            type == PortType::In ? "in":"out");
        };

        auto portId = root->portId(type, idx);
        if (!portId.isValid())
        {
            gtError() << makeError() << tr("(Source port not found)");
            return;
        }

        if (!provider)
        {
            gtError() << makeError() << tr("(provider not found)");
            return;
        }

        if (!provider->removePort(portId))
        {
            gtError() << makeError()
                      << QObject::tr("(Removing port of provider failed)");
            return;
        }
    }

    /// Functor to handle port deletion
    struct PortDeleted
    {
        PortDeleted(Graph* g, Node* n) : graph(g), node(n)
        {
            assert(graph);
            assert(node);
        }

        void operator()(PortType type, PortIndex idx)
        {
            NodeId nodeId = node->id();

            auto port = node->portId(type, idx);
            if (port == invalid<PortId>())
            {
                gtWarning() << tr("Failed to update connections of deleted "
                                  "port %1 with %2 of node %3!")
                                   .arg(port).arg(toString(type)).arg(nodeId);
                return;
            }

            emit graph->nodePortAboutToBeDeleted(nodeId, type, idx);

            auto connections = graph->connectionModel().iterateConnections(nodeId, port);
            if (connections.empty()) return;

            auto cmd = graph->modify();
            for (auto conId : connections)
            {
                graph->deleteConnection(conId);
            }
        }

    private:

        Graph* graph = nullptr;
        Node* node = nullptr;
    };

    /// Functor to handle port change
    struct PortChanged
    {
        PortChanged(Graph* g, Node* n) : graph(g), node(n)
        {
            assert(graph);
            assert(node);
        }

        void operator()(PortId portId)
        {
            NodeId nodeId = node->id();

            PortInfo* port = node->port(portId);
            if (!port)
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to update connections of changed "
                                  "portId %1 node %2!")
                                   .arg(portId).arg(nodeId);
                return;
            }

            auto& conModel = graph->connectionModel();
            auto connections = conModel.iterateConnections(nodeId, portId);

            bool isConnected = !connections.empty();
            if (port->isConnected() != isConnected)
            {
                isConnected ? emit node->portConnected(port->id()) :
                              emit node->portDisconnected(port->id());
            }

            if (!isConnected) return;

            PortType type = invert(node->portType(portId));
            assert(type != PortType::NoType);

            Modification cmd;
            assert(cmd.isNull());

            auto& factory = NodeDataFactory::instance();

            // check if connections are still valid
            for (auto conId : connections)
            {
                NodeId otherNodeId = conId.node(type);
                assert(otherNodeId != nodeId);

                Node const* otherNode = conModel.node(otherNodeId);
                if (!otherNode) continue;

                PortInfo const* otherPort = otherNode->port(conId.port(type));
                if (!otherPort) continue;

                if (!factory.canConvert(port->typeId, otherPort->typeId))
                {
                    if (cmd.isNull()) cmd = graph->modify();
                    graph->deleteConnection(conId);
                }
            }
        }

    private:

        Graph* graph = nullptr;
        Node* node = nullptr;
    };

    /// Functor to handle node deletion
    struct NodeDeleted
    {
        NodeDeleted(Graph* g) : graph(g)
        {
            assert(graph);
        }

        void operator()(NodeId nodeId)
        {
            auto localIter = graph->pimpl->local.find(nodeId);
            if (localIter == graph->pimpl->local.end())
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete node") << nodeId
                            << tr("(node was not found!)");
                return;
            }
            Node* node = localIter->node;
            assert(node);
            auto const& nodeUuid = node->uuid();

            auto globalIter = graph->pimpl->global->find(nodeUuid);
            if (globalIter == graph->pimpl->global->end())
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete node") << nodeId
                            << tr("(node was not found in global model!)");
                return;
            }

            auto* root = graph->rootGraph();
            assert(root);
            assert(root && root->pimpl->global.get() == graph->pimpl->global.get());
            if (root->pimpl->global.get() != graph->pimpl->global.get())
            {
                gtError() << root << ":" << root->pimpl->global.get() << "vs"
                          << graph << ":" << graph->pimpl->global.get();
            }

            auto change = graph->modify();
            Q_UNUSED(change);

            // remove local connections
            for (auto conId : localIter->iterateConnections().reverse())
            {
                graph->deleteConnection(conId);
            }
            // remove remaining global connections
            for (auto conId : globalIter->iterateConnections().reverse())
            {
                GlobalConnectionDeleted{graph, conId}();
            }

            emit graph->childNodeAboutToBeDeleted(nodeId);
            
            graph->pimpl->local.erase(localIter);
            graph->pimpl->global->erase(globalIter);
            
            emit graph->childNodeDeleted(nodeId);
        }

    private:

        Graph* graph = nullptr;
    };

    /// Common base class to handle deletion of a connection
    template <typename NodeId_t>
    struct ConnectionDeletedCommon
    {
        ConnectionDeletedCommon(Graph* g,
                                ConnectionModel_t<NodeId_t>* m,
                                ConnectionId_t<NodeId_t> id) :
            graph(g), model(m), conId(id)
        {
            assert(graph);
            assert(model);
            assert(conId.isValid());
            assert(conId.inNodeId != conId.outNodeId);
        }

        bool operator()()
        {
            auto inConnection  = ConnectionDetail<NodeId_t>::fromConnection(conId.reversed());
            auto outConnection = ConnectionDetail<NodeId_t>::fromConnection(conId);

            auto targetNode = model->find(conId.inNodeId);
            auto sourceNode = model->find(conId.outNodeId);

            if (targetNode == model->end() || sourceNode == model->end())
            {
                // global connection may already be deleted, but this function
                // may still be triggered causing a false alarm
                gtWarning().verbose()
                    << utils::logId(*graph)
                    << tr("Failed to delete connection %1")
                           .arg(toString(conId))
                    << tr("(in-node entry %1, out-node entry %2!)")
                           .arg(targetNode != model->end() ? "found" : "not found")
                           .arg(sourceNode != model->end() ? "found" : "not found");
                return false;
            }

            assert(targetNode->node &&
                   get_node_id<NodeId_t>{}(targetNode->node) == conId.inNodeId);
            assert(sourceNode->node &&
                   get_node_id<NodeId_t>{}(sourceNode->node) == conId.outNodeId);

            auto inIdx  = targetNode->predecessors.indexOf(inConnection);
            auto outIdx = sourceNode->successors.indexOf(outConnection);

            if (inIdx < 0 || outIdx < 0)
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete connection %1")
                                   .arg(toString(conId))
                            << tr("(in-connection %1, out-connection %2!)")
                                   .arg(inIdx  >= 0 ? "found" : "not found")
                                   .arg(outIdx >= 0 ? "found" : "not found");
                return false;
            }

            targetNode->predecessors.remove(inIdx);
            sourceNode->successors.remove(outIdx);

            // update ports once if local model changes
            if (std::is_same<NodeId, NodeId_t>::value)
            {
                auto* inPort = targetNode->node->port(conId.inPort);
                auto* outPort = sourceNode->node->port(conId.outPort);
                assert(inPort);
                assert(outPort);

                // input port should have no connections
                assert(targetNode->iterateConnections(inPort->id()).empty());
                emit targetNode->node->portDisconnected(inPort->id());

                // output port may still be connected
                bool isConnected = !sourceNode->iterate(outPort->id()).empty();
                if (!isConnected) emit sourceNode->node->portDisconnected(outPort->id());
            }

            return true;
        }

    protected:

        Graph* graph = nullptr;
        ConnectionModel_t<NodeId_t>* model;
        ConnectionId_t<NodeId_t> conId;
    };

    /// Functor to handle deletion of a "global" connection
    struct GlobalConnectionDeleted : public ConnectionDeletedCommon<NodeUuid>
    {
        GlobalConnectionDeleted(Graph* g, ConnectionUuid id) :
            ConnectionDeletedCommon(g, g->pimpl->global.get(), std::move(id))
        { }

        void operator()()
        {
            model = graph->pimpl->global.get(); // update ptr
            if (ConnectionDeletedCommon<NodeUuid>::operator()())
            {
                emit graph->globalConnectionDeleted(conId);
            }
        }
    };

    /// Functor to handle deletion of a "local" connection
    struct ConnectionDeleted : public ConnectionDeletedCommon<NodeId>
    {
        ConnectionDeleted(Graph* g, ConnectionId id) :
            ConnectionDeletedCommon(g, &g->pimpl->local, std::move(id))
        { }

        void operator()()
        {
            if (ConnectionDeletedCommon<NodeId>::operator()())
            {
                emit graph->connectionDeleted(conId);
            }
        }
    };

}; // struct Impl

} // namespace intelli

#endif
