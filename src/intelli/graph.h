/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_GRAPH_H
#define GT_INTELLI_GRAPH_H

#include <intelli/node.h>

#include <gt_finally.h>
#include <gt_platform.h>

#include <QPointer>

class GtMdiItem;

namespace intelli
{

class Graph;
class GroupInputProvider;
class GroupOutputProvider;
class Connection;
class ConnectionGroup;
class DynamicNode;

/**
 * @brief Opens the graph in a graph editor. The graph object should be kept
 * alive to keep the editor open
 * @param graph Graph to open
 * @return ´Pointer to mdi item
 */
GT_INTELLI_EXPORT
GtMdiItem* show(Graph& graph);

/**
 * @brief Opens the graph in a graph editor. Movees the ownership to the graph
 * editor.
 * @param graph Graph to open
 * @return pointer to mdi item
 */
GT_INTELLI_EXPORT
GtMdiItem* show(std::unique_ptr<Graph> graph);

/**
 * @brief Returns the cyclic nodes of the given graph. If the returned list is
 * empty the graph is acyclic. if not the returned sequence does contain a cycle
 * however not all nodes may be part of this cylce.
 * @param graph Graph to check for cycles
 * @return Returns true if graph is acyclic otherwise returns false
 */
GT_INTELLI_EXPORT
QVector<NodeId> cyclicNodes(Graph const& graph);

/**
 * @brief Returns whether a graph is acyclic (i.e. does not contain loops/cylces)
 * @param graph Graph to check for cycles
 * @return Returns true if graph is acyclic otherwise returns false
 */
inline bool isAcyclic(Graph const& graph) { return cyclicNodes(graph).empty(); }

/**
 * @brief Policy for handling node id collisions, when appending a node to a graph
 */
enum class NodeIdPolicy
{
    /// Indictaes that the node id may be updated if it already exists
    Update = 0,
    /// Indicates that the node id should not be updated.
    Keep = 1
};

/// directed acyclic graph representing connections and nodes
namespace connection_model
{

template <typename NodeId_t>
struct ConnectionDetail
{
    /// target node
    NodeId_t node;
    /// target port
    PortId port;
    /// source port
    PortId sourcePort;

    /**
     * @brief Creates an outgoing connection id.
     * @param sourceNode Source (outgoing) node
     * @return Connection id
     */
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode) const
    {
        return { sourceNode, sourcePort, node, port };
    }

    /**
     * @brief Constructs an object from an outgoing connection
     * @param conId Source connection
     * @return Connection detail
     */
    static ConnectionDetail fromConnection(ConnectionId_t<NodeId_t> conId)
    {
        return { conId.inNodeId, conId.inPort, conId.outPort };
    }
};

template <typename NodeId_t>
struct ConnectionData
{
    static constexpr size_t PRE_ALLOC = 10;

    /// pointer to node
    QPointer<Node> node;
    /// adjacency lists
    QVarLengthArray<ConnectionDetail<NodeId_t>, PRE_ALLOC> predecessors = {}, successors = {};

    /**
     * @brief Returns the predecessors or successors depending on the port type
     * @param type Port type
     * @return Port vector
     */
    auto& ports(PortType type)
    {
        assert(type != PortType::NoType);
        return (type == PortType::In) ? predecessors : successors;
    }
    auto const& ports(PortType type) const
    {
        return const_cast<ConnectionData*>(this)->ports(type);
    }
};

template <typename T>
using ConnectionModel = QHash<NodeId, ConnectionData<T>>;

template <typename Model, typename NodeId_t>
inline auto* find(Model& model, NodeId_t nodeId)
{
    using T = decltype(&(*model.begin()));

    auto iter = model.find(nodeId);
    if (iter == model.end()) return T(nullptr);
    return &(*iter);
}


// operators
template <typename T>
inline bool operator==(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b)
{
    return a.node == b.node && a.port == b.port && a.sourcePort == b.sourcePort;
}

template <typename T>
inline bool operator!=(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b) { return !(a == b); }

// definitions

using GlobalConnectionGraph = ConnectionModel<NodeUuid>;

using ConnectionGraph = ConnectionModel<NodeId>;
using DirectedAcyclicGraph [[deprecated]] = ConnectionGraph;

[[deprecated("use `debug` overload")]]
GT_INTELLI_EXPORT void debugGraph(ConnectionGraph const&);

} // namespace connection_model

namespace dag = connection_model;

/// prints the graph as a mermaid flow chart useful for debugging
GT_INTELLI_EXPORT void debug(Graph const& graph);

using connection_model::ConnectionGraph;
using DirectedAcyclicGraph [[deprecated]] = ConnectionGraph;

using connection_model::GlobalConnectionGraph;

/**
 * @brief The Graph class.
 * Represents an entire intelli graph and manages all its nodes and connections.
 * Can be appended itself to another graph. Prefer to use appendNode and
 * appendConnection functions instead of using appendChild or setParent.
 */
class GT_INTELLI_EXPORT Graph : public Node
{
    Q_OBJECT

    template <PortType>
    friend class AbstractGroupProvider;
    friend class GroupInputProvider;

    friend class GraphBuilder;
    friend class GraphScene;

public:

    Q_INVOKABLE Graph();
    ~Graph();

    using Node::portId;

    /**
     * @brief Overload that accepts a node. Will search for the node and return
     * the port id of the port specified port its type and index. An invalid
     * port id is returned if the node was not found or the port is out of bounds.
     * @param nodeId Node to access port id of
     * @param type Port type
     * @param portIdx Port index
     * @return port id (may be invalid)
     */
    PortId portId(NodeId nodeId, PortType type, PortIndex portIdx) const;

    /**
     * @brief Returns the connection id matched by the given nodes and port
     * indicies. This can be used to easily access a connection id without
     * knowing the specific port ids. The returned connection id may be invalid
     * if the port are out of bounds or a node does not exist.
     * @param outNodeId Starting node id
     * @param outPortIdx Starting port index
     * @param inNodeId Ending node id
     * @param inPortIdx Ending port index
     * @return Connection id (may be invalid)
     */
    ConnectionId connectionId(NodeId outNodeId, PortIndex outPortIdx,
                              NodeId inNodeId, PortIndex inPortIdx) const;

    /**
     * @brief Returns the parent graph of this node (null if the graph is a
     * root graph node)
     * @return Parent graph
     */
    Graph* parentGraph();
    Graph const* parentGraph() const;

    /**
     * @brief Returns the root graph of this graph. If no root is found it is
     * assumed that this object is the root.
     * @return Root graph
     */
    Graph* rootGraph();
    Graph const* rootGraph() const;

    /**
     * @brief Returns a list of all nodes in this graph
     * @return Nodes
     */
    QList<Node*> nodes();
    QList<Node const*> nodes() const;

    /**
     * @brief Returns a list of all node ids in this graph
     * @return Node ids
     */
    QVector<NodeId> nodeIds() const;
    
    /**
     * @brief Returns a list of all connections in this graph
     * @return Connections
     */
    QList<Connection*> connections();
    QList<Connection const*> connections() const;

    /**
     * @brief Returns a list of all connection ids in this graph
     * @return Connection ids
     */
    QVector<ConnectionId> connectionIds() const;
    
    /**
     * @brief Attempts to finde the node specified by the given nodeId
     * @param nodeId node id
     * @return node matched by nodeId (null if node was not found)
     */
    Node* findNode(NodeId nodeId);
    Node const* findNode(NodeId nodeId) const;

    Node* findNodeByUuid(NodeUuid const& uuid);
    Node const* findNodeByUuid(NodeUuid const& uuid) const;

    /**
     * @brief Attempts to finde a connection specified by the given connectionId
     * @param conId Connection id
     * @return connection matched by conId (null if connection was not found)
     */
    Connection* findConnection(ConnectionId conId);
    Connection const* findConnection(ConnectionId conId) const;

    /**
     * @brief Finds all connections associated with the node specified by node id.
     * The connections can be narrowed down to ingoing and outgoing connection
     * @param nodeId Node id
     * @param type Connection filter (in/out going only or both)
     * @return Connections
     */
    QVector<ConnectionId> findConnections(NodeId nodeId, PortType type = PortType::NoType) const;

    /**
     * @brief Overload. Finds all connections associated with the node and its
     * port id.
     * @param nodeId Node id
     * @param portId Port id
     * @return Connections
     */
    QVector<ConnectionId> findConnections(NodeId nodeId, PortId portId) const;

    /**
     * @brief Finds all connected nodes. I.e. ancestors or descendants. A node
     * is not listed twice.
     * @param nodeId Node id
     * @param type Connection filter (in/out going only or both)
     * @return Connections
     */
    QVector<NodeId> findConnectedNodes(NodeId nodeId, PortType type = PortType::NoType) const;

    /**
     * @brief Overload. Finds all nodes that are connected to the source node
     * and port specified.
     * @param nodeId Node id
     * @param portId Port id
     * @return Connections
     */
    QVector<NodeId> findConnectedNodes(NodeId nodeId, PortId portId) const;

    /**
     * @brief Static method to that returns all target nodes without duplicates
     * @param connections Connections
     * @param type The type of connections, i.e. are these outgoing connections?
     * Then the target nodes are the ones on the in going/recieving side and
     * vice cersa.
     * @return List of all unique target nodes
     */
    static QVector<NodeId> uniqueTargetNodes(QVector<ConnectionId> const& connections, PortType type);

    /**
     * @brief Returns a list of all sub graphes (aka group nodes)
     * @return List of sub graphs
     */
    QList<Graph*> graphNodes();
    QList<Graph const*> graphNodes() const;
    
    /**
     * @brief Returns the input provider of this graph. May be null if sub graph
     * was not yet initialized or it is the root graph.
     * @return Input provider
     */
    GroupInputProvider* inputProvider();
    GroupInputProvider const* inputProvider() const;

    DynamicNode* inputNode();
    DynamicNode const* inputNode() const;

    /**
     * @brief Returns the output provider of this graph. May be null if sub graph
     * was not yet initialized or it is the root graph.
     * @return Output provider
     */
    GroupOutputProvider* outputProvider();
    GroupOutputProvider const* outputProvider() const;

    DynamicNode* outputNode();
    DynamicNode const* outputNode() const;

    /**
     * @brief Finds all dependencies of the node referred by `nodeId`
     * @param nodeId Node to find dependencies of
     * @return Dependencies
     */
    QVector<NodeId> findDependencies(NodeId nodeId) const;

    /**
     * @brief Finds all nodes that are depent of the node referred by `nodeId`
     * @param nodeId Node to find depent nodes of
     * @return Dependent nodes
     */
    QVector<NodeId> findDependentNodes(NodeId nodeId) const;

    /**
     * @brief Clears all nodes and connections
     */
    void clearGraph();

    /**
     * @brief Checks if the connection `conid` can be appended to the graph
     * @param conId Connection to chech
     * @return Can connect
     */
    bool canAppendConnections(ConnectionId conId);

    /**
     * @brief Appends the node to the intelli graph. Use this function instead
     * of appending the child directly. Node may change its id if its
     * already occupied
     * @param node Node to append
     * @param policy Whether to generate a new id if necessary
     * @return success
     */
    Node* appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Appends the connection to intelli graph. Use this function instead
     * of appending the child directly. Aborts if the connection already exists
     * @param connection Connection to append
     * @return success
     */
    Connection* appendConnection(std::unique_ptr<Connection> connection);
    
    /**
     * @brief Appends the nodes and connections to the intelli graph. Useful when
     * bulk inserting many nodes and connetions. If a node id already exists it
     * and the affected connections will be updated. A list of all new node ids
     * will be returned in the same order.
     *
     * Note: If the operation fails the returned list contains only the elements
     * that have been added sucessfully. Check the size of the returned list to
     * check if operation failed. The object does not cleanup the added nodes
     * and connections in such a case.
     * @param nodes Nodes to insert. May change node ids
     * @param connections Connections to insert
     * @return List of new node ids. The order is kept. The operation failed,
     * if the size does not match the input size
     */
    QVector<NodeId> appendObjects(std::vector<std::unique_ptr<Node>>& nodes,
                                  std::vector<std::unique_ptr<Connection>>& connections);

    /**
     * @brief Deletes the node specified by node id. Returns true on success.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteNode(NodeId nodeId);

    /**
     * @brief Deletes the connection specified by the connection id.
     * Returns true on success.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteConnection(ConnectionId connectionId);

    /**
     * @brief Access the directed acyclic graph model used to manage the nodes
     * and their connections
     * @return DAG
     */
    [[deprecated("use `data` instead")]]
    ConnectionGraph const& dag() const { return m_data; }

    /**
     * @brief Gives access to the internal graph model used to manage the nodes
     * and their connections
     * @return Graph model
     */
    inline auto const& data() const { return m_data; }

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders();

    struct EndModificationFunctor
    {
        inline void operator()() const noexcept
        {
            if (graph) graph->emitEndModification();
        }
        Graph* graph{};
    };

    using Modification = gt::Finally<EndModificationFunctor>;

    /**
     * @brief Scopped wrapper around `beginModification` and `endModification`
     * @return Scoped object which emits the begin and end modification signals
     * resprectively, used to signal that evaluation and similar processes
     * should be halted.
     */
    GT_NO_DISCARD
    Modification modify();

    /**
     * @brief Tells the graph that is about to be modifed. Should be called
     * before e.g. bulk deleting/inserting nodes and connections
     */
    void emitBeginModification();

    /**
     * @brief Tells the graph that is no longer beeing modifed. Should be called
     * after e.g. bulk deleting/inserting nodes and connections
     */
    void emitEndModification();

signals:

    /**
     * @brief Emitted just before ths object is deleted. Thus, specific memebers
     * of this object are still accessable.
     */
    void graphAboutToBeDeleted(QPrivateSignal);

    /**
     * @brief Emitted once the graph is beeing modified.
     */
    void beginModification(QPrivateSignal);

    /**
     * @brief Emitted once the graph is no longer beeing modified
     */
    void endModification(QPrivateSignal);

    /**
     * @brief Emitted once a connection was appended
     * @param Pointer to connection object
     */
    void connectionAppended(Connection* connection);

    /**
     * @brief Emitted after a conections was deleted
     * @param Connection id of the deleted connection
     */
    void connectionDeleted(ConnectionId connectionId);

    /**
     * @brief Forwards the `portInserted` signal of node, so that
     * other components can react before the graph does.
     * @param nodeId Affected Node
     * @param type Type of the deleted port
     * @param idx Index of the deleted port
     */
    void nodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    /**
     * @brief Forwards the `portAboutToBeDeleted` signal of node, so that
     * other components can react before the graph does.
     * @param nodeId Affected Node
     * @param type Type of the deleted port
     * @param idx Index of the deleted port
     */
    void nodePortAboutToBeDeleted(NodeId nodeId, PortType type, PortIndex idx);

    /**
     * @brief Forwards the `portDeleted` signal of node, so that
     * other components can react before the graph does.
     * @param nodeId Affected Node
     * @param type Type of the deleted port
     * @param idx Index of the deleted port
     */
    void nodePortDeleted(NodeId nodeId, PortType type, PortIndex idx);
    
    /**
     * @brief Emitted once a node was appended
     * @param Pointer to node object
     */
    void nodeAppended(Node* node);

    /**
     * @brief Emitted after a node was deleted
     * @param Node id of the deleted node
     */
    void childNodeAboutToBeDeleted(NodeId nodeId);

    /**
     * @brief Emitted after a node was deleted
     * @param Node id of the deleted node
     */
    void childNodeDeleted(NodeId nodeId);

    /**
     * @brief Emitted once the position of a node was altered. Is only triggered
     * by calling `setNodePosition`.
     * @param nodeId NodeId that changed the position
     * @param pos New position
     */
    void nodePositionChanged(NodeId nodeId, QPointF pos);

protected:
    
    void eval() override;

    void onObjectDataMerged() override;

private:

    struct Impl;

    /// connection graph
    ConnectionGraph m_data;

    size_t m_evaluationIndicator = 0;
    /// indicator if the connection model is currently beeing modified
    int m_modificationCount = 0;

    /**
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    ConnectionGroup& connectionGroup();
    ConnectionGroup const& connectionGroup() const;

    void restoreNode(Node* node);
    void restoreConnection(Connection* connection);

    void restoreConnections();
    void restoreNodesAndConnections();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPH_H
