#ifndef GT_INTELLI_GRAPH_IMPL_H
#define GT_INTELLI_GRAPH_IMPL_H

#include <intelli/graph.h>
#include <intelli/nodedatafactory.h>
#include <intelli/private/utils.h>

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
        auto* targetNode = connection_model::find(graph.m_local, conId.inNodeId);
        auto* sourceNode = connection_model::find(graph.m_local, conId.outNodeId);

        if (!targetNode || !sourceNode)
        {
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(connection in-node %1, out-node %2)")
                                   .arg(targetNode ? "found" : "not found")
                                   .arg(sourceNode ? "found" : "not found");
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
        auto& conModel = graph.connectionModel();
        auto* conData = connection_model::find(conModel, conId.inNodeId);
        assert(conData);

        bool connected = connection_model::hasConnections(*conData, conId.inPort);
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
        auto const* entry = connection_model::find(graph.m_local, nodeId);
        if (!entry) return false;

        for (auto& dependent : entry->ports(type))
        {
            if (nodes.contains(dependent.node)) continue;
            nodes.append(dependent.node);
            if (!accumulateDependentNodes(graph, nodes, dependent.node, type))
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

            auto conData = graph->connectionModel().find(nodeId);
            if (conData == graph->connectionModel().end()) return;

            auto cons = conData->iterateConnections(port);
            if (cons.empty()) return;

            auto cmd = graph->modify();
            for (auto conId : cons)
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

            auto conData = graph->connectionModel().find(nodeId);
            if (conData == graph->connectionModel().end()) return;

            auto cons = conData->iterateConnections(portId);
            if (cons.empty()) return;

            PortInfo* port = node->port(portId);
            if (!port)
            {
                gtWarning() << tr("Failed to update connections of changed "
                                  "portId %1 node %2!")
                                   .arg(portId).arg(nodeId);
                return;
            }

            PortType type = invert(node->portType(portId));
            assert(type != PortType::NoType);

            Modification cmd;
            assert(cmd.isNull());

            auto& factory = NodeDataFactory::instance();

            // check if connections are still valid
            for (auto conId : cons)
            {
                NodeId otherNodeId = conId.node(type);
                assert(otherNodeId != nodeId);

                Node* otherNode = graph->findNode(otherNodeId);
                if (!otherNode) continue;

                PortInfo* otherPort = otherNode->port(conId.port(type));
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
                gtWarning() << tr("Failed to delete node") << nodeId
                            << tr("(node was not found!)");
                return;
            }
            Node* node = localIter->node;
            assert(node);
            auto const& nodeUuid = node->uuid();

            auto globalIter = graph->m_global->find(nodeUuid);
            if (globalIter == graph->m_global->end())
            {
                gtWarning() << tr("Failed to delete node") << nodeId
                            << tr("(node was not found in global model!)");
                return;
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
                                ConnectionModel<NodeId_t>* m,
                                ConnectionId_t<NodeId_t> id) :
            graph(g), model(m), conId(id)
        {
            assert(graph);
            assert(model);
            assert(conId.isValid());
        }

        bool operator()()
        {
            using namespace connection_model;

            auto inConnection  = ConnectionDetail<NodeId_t>::fromConnection(conId.reversed());
            auto outConnection = ConnectionDetail<NodeId_t>::fromConnection(conId);

            auto* targetNode = connection_model::find(*model, conId.inNodeId);
            auto* sourceNode = connection_model::find(*model, conId.outNodeId);

            if (!targetNode || !sourceNode)
            {
                gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                            << tr("(in-node %1, out-node %2!)")
                                   .arg(targetNode ? "found" : "not found")
                                   .arg(sourceNode ? "found" : "not found");
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
                gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                            << tr("(in-connection %1, out-connection %2!)")
                                   .arg(inIdx  >= 0 ? "found" : "not found")
                                   .arg(outIdx >= 0 ? "found" : "not found");
                return false;
            }

            if (std::is_same<NodeId, NodeId_t>::value)
            {
                emit targetNode->node->portDisconnected(conId.inPort);
                emit sourceNode->node->portDisconnected(conId.outPort);
            }

            targetNode->predecessors.remove(inIdx);
            sourceNode->successors.remove(outIdx);

            return true;
        }

    protected:

        Graph* graph = nullptr;
        ConnectionModel<NodeId_t>* model;
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
