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
#include <intelli/nodedatafactory.h>
#include <intelli/private/utils.h>
#include <intelli/connection.h>

#include <gt_logging.h>

namespace intelli
{

/// Helper struct to "hide" implementation details and template functions
struct Graph::Impl
{
    template <typename MakeError = QString(*)()>
    static inline bool
    canAppendConnection(Graph& graph,
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
            return {};
        }

        // check if nodes differ
        if (conId.inNodeId == conId.outNodeId)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node and out-node are qeual)");
            }
            return {};
        }

        // connection may already exist
        if (graph.findConnection(conId))
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection already exists)");
            }
            return {};
        }

        // check if nodes exist
        auto& conModel = graph.connectionModel();

        auto targetNode = conModel.find(conId.inNodeId);;
        auto sourceNode = conModel.find(conId.outNodeId);

        if (targetNode == conModel.end() || sourceNode == conModel.end())
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node %1, out-node %2)")
                                   .arg(targetNode == conModel.end() ? "found" : "not found")
                                   .arg(sourceNode == conModel.end() ? "found" : "not found");
            }
            return {};
        }

        assert(targetNode->node->id() == conId.inNodeId &&
               targetNode->node->parent()  == &graph);
        assert(sourceNode->node->id() == conId.outNodeId &&
               sourceNode->node->parent() == &graph);

        // check if ports to connect exist
        auto inPort  = targetNode->node->port(conId.inPort);
        auto outPort = sourceNode->node->port(conId.outPort);

        if (!inPort || !outPort)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-port %1, out-port %2)")
                                   .arg(inPort  ? "found" : "not found")
                                   .arg(outPort ? "found" : "not found");
            }
            return {};
        }

        // check if output is connected to input
        if (targetNode->node->portType(inPort->id())  ==
            sourceNode->node->portType(outPort->id()))
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(cannot connect ports of same port type)");
            }
            return {};
        }

        // target node should be an input port
        assert (targetNode->node->portType(inPort->id()) == PortType::In);

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
            return {};
        }

        // check if input port is already connected
        auto connected = !targetNode->iterateConnections(conId.inPort).empty();
        if (connected)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(in-port is already connected)");
            }
            return {};
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
        for (auto const& entry : graph.m_local)
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
            auto maxId = ids.empty() ? 0 : *std::max_element(std::begin(ids), std::end(ids)) + 1;
            node.setId(NodeId::fromValue(maxId));

            return node.id() != invalid<NodeId>();
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
        for (auto& entry : graph.m_local)
        {
            graph.m_global->insert(entry.node->uuid(), entry.node);
        }
        // recurisvely append nodes and connections
        for (Graph* subgraph : graph.graphNodes())
        {
            assert(graph.m_global.get() == subgraph->m_global.get());
            repopulateGlobalConnectionModel(*subgraph);
        }
        // append connections of this graph
        for (auto& entry : graph.m_local)
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
            auto localIter = graph->m_local.find(nodeId);
            if (localIter == graph->m_local.end())
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete node") << nodeId
                            << tr("(node was not found!)");
                return;
            }
            Node* node = localIter->node;
            assert(node);
            auto const& nodeUuid = node->uuid();

            gtDebug() << relativeNodePath(*graph)
                      << "DELETING NODE" << node << nodeId << nodeUuid;

            auto globalIter = graph->m_global->find(nodeUuid);
            if (globalIter == graph->m_global->end())
            {
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete node") << nodeId
                            << tr("(node was not found in global model!)");
                return;
            }

            auto* root = graph->rootGraph();
            assert(root && root->m_global.get() == graph->m_global.get());

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
            
            graph->m_local.erase(localIter);
            graph->m_global->erase(globalIter);
            
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
                gtWarning() << utils::logId(*graph)
                            << tr("Failed to delete connection %1")
                                   .arg(toString(conId))
                            << tr("(in-node entry %1, out-node entry %2!)")
                                   .arg(targetNode != model->end() ? "found" : "not found")
                                   .arg(sourceNode != model->end() ? "found" : "not found");
                return false;
            }

            gtDebug() << "###" << graph->caption() << conId << "\n      "
                      << (void*)targetNode->node << targetNode->node << (targetNode->node ? get_node_id<NodeId_t>{}(targetNode->node) : NodeId_t()) << "VS" << conId.inNodeId << "\n      "
                      << (void*)sourceNode->node << sourceNode->node << (sourceNode->node ? get_node_id<NodeId_t>{}(sourceNode->node) : NodeId_t()) << "VS" << conId.outNodeId;

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
            ConnectionDeletedCommon(g, g->m_global.get(), std::move(id))
        { }

        void operator()()
        {
            model = graph->m_global.get(); // update ptr
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
            ConnectionDeletedCommon(g, &g->m_local, std::move(id))
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
