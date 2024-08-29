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
                            << tr("(connection in-node or out-node was not found)");
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
                            << tr("(connection in-port or out-port not found)");
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
                            << tr("(cannot connect ports with incomaptible types)");
            }
            return {};
        }

        // check if input port is already connected
        auto const& cons = graph.findConnections(conId.inNodeId, conId.inPort);
        if (!cons.empty())
        {
            assert (cons.size() == 1);
            if (!silent)
            {
                gtWarning() << makeError()
                            << tr("(in-port is already connected to '%1')").arg(toString(cons.first()));
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

            auto const& connections = graph->findConnections(nodeId, port);
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

            auto const& connections = graph->findConnections(nodeId, portId);
            if (connections.empty()) return;

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
            for (auto conId : connections)
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
            auto node = graph->m_local.find(nodeId);
            if (node == graph->m_local.end())
            {
                gtWarning() << tr("Failed to delete node") << nodeId
                            << tr("(node was not found!)");
                return;
            }

            auto cmd = graph->modify();

            auto const& connections = graph->findConnections(nodeId);
            for (auto conId : connections)
            {
                graph->deleteConnection(conId);
            }

            emit graph->childNodeAboutToBeDeleted(nodeId);
            
            graph->m_local.erase(node);
            
            emit graph->childNodeDeleted(nodeId);
        }

    private:

        Graph* graph = nullptr;
    };

    template <typename NodeId_t>
    struct GetModel
    {
        LocalConnectionModel& operator()(Graph* graph) { return graph->m_local; }
    };
    template <>
    struct GetModel<NodeUuid>
    {
        GlobalConnectionModel& operator()(Graph* graph) { return *graph->m_global; }
    };
    template <typename NodeId_t>
    struct GetGraph
    {
        Graph* operator()(Node* node) { return static_cast<Graph*>(node->parent()); }
    };
    template <>
    struct GetGraph<NodeUuid>
    {
        Graph* operator()(Node* node) { return static_cast<Graph*>(node->parent())->rootGraph(); }
    };

    template <typename NodeId_t>
    struct GetNodeId
    {
        NodeId operator()(Node* node) { return node->id(); }
    };
    template <>
    struct GetNodeId<NodeUuid>
    {
        NodeUuid operator()(Node* node) { return node->uuid(); }
    };

    /// Common base class to handle deletion of a connection
    template <typename NodeId_t>
    struct ConnectionDeletedCommon
    {
        ConnectionDeletedCommon(Graph* g, ConnectionId_t<NodeId_t> id) :
            graph(g), conId(id)
        {
            assert(graph);
            assert(conId.isValid());
        }

        bool operator()()
        {
            using namespace connection_model;

            auto inConnection  = ConnectionDetail<NodeId_t>::fromConnection(conId.reversed());
            auto outConnection = ConnectionDetail<NodeId_t>::fromConnection(conId);

            auto* targetNode = connection_model::find(GetModel<NodeId_t>{}(graph), conId.inNodeId);
            auto* sourceNode = connection_model::find(GetModel<NodeId_t>{}(graph), conId.outNodeId);

            if (!targetNode || !sourceNode)
            {
                gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                            << tr("(in-node or out-node was not found!)");
                return false;
            }

            assert(targetNode->node &&
                   GetNodeId<NodeId_t>{}(targetNode->node) == conId.inNodeId);
            assert(sourceNode->node &&
                   GetNodeId<NodeId_t>{}(sourceNode->node) == conId.outNodeId);

            auto inIdx  = targetNode->predecessors.indexOf(inConnection);
            auto outIdx = sourceNode->successors.indexOf(outConnection);

            if (inIdx < 0 || outIdx < 0)
            {
                gtWarning() << tr("Failed to delete connection %1").arg(toString(conId))
                            << tr("(in-connection and out-connection was not found!)")
                            << "in:" << (inIdx >= 0) << "and out:" << (outIdx >= 0);
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
        ConnectionId_t<NodeId_t> conId;
    };

    /// Functor to handle deletion of a "global" connection
    struct GlobalConnectionDeleted : public ConnectionDeletedCommon<NodeUuid>
    {
        using ConnectionDeletedCommon<NodeUuid>::ConnectionDeletedCommon;

        void operator()()
        {
            return (void)ConnectionDeletedCommon<NodeUuid>::operator()();
        }
    };

    /// Functor to handle deletion of a "local" connection
    struct ConnectionDeleted : public ConnectionDeletedCommon<NodeId>
    {
        using ConnectionDeletedCommon<NodeId>::ConnectionDeletedCommon;

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
