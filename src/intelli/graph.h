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

#include <QPointer>

class GtMdiItem;

namespace intelli
{

class Graph;
class GroupInputProvider;
class GroupOutputProvider;
class Connection;
class ConnectionGroup;
class ModelAdapter;

/**
 * @brief Evaluates the given graph. This call is blocking. If the graph is
 * already active it cannot be evaluated and the false is returned. The results
 * can be access by iterating over each node and accessing `outData`.
 * @param graph Graph to execute
 * @return Whether the graph was evaluated sucessfully
 */
GT_INTELLI_EXPORT
bool evaluate(Graph& graph);
GT_INTELLI_EXPORT
bool evaluate(Graph& graph, GraphExecutionModel& model);

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
QVector<NodeId> cyclicNodes(Graph& graph);

/**
 * @brief Returns whether a graph is acyclic (i.e. does not contain loops/cylces)
 * @param graph Graph to check for cycles
 * @return Returns true if graph is acyclic otherwise returns false
 */
inline bool isAcyclic(Graph& graph) { return cyclicNodes(graph).empty(); }

/// directed acyclic graph
namespace dag
{

struct ConnectionDetail
{
    /// target node
    NodeId node;
    /// target port
    PortIndex port;
    /// source port
    PortIndex sourcePort;

    /**
     * @brief Creates an outgoing connection id.
     * @param sourceNode Source (outgoing) node
     * @return Connection id
     */
    ConnectionId toConnection(NodeId sourceNode) const
    {
        return { sourceNode, sourcePort, node, port };
    }

    /**
     * @brief Constructs an object from an outgoing connection
     * @param conId Source connection
     * @return Connection detail
     */
    static ConnectionDetail fromConnection(ConnectionId conId)
    {
        return { conId.inNodeId, conId.inPortIndex, conId.outPortIndex };
    }
};

struct Entry
{
    /// pointer to node
    QPointer<Node> node;
    /// adjacency lists
    QVarLengthArray<ConnectionDetail, 12> ancestors = {}, descendants = {};
};

inline bool operator==(ConnectionDetail const& a, ConnectionDetail const& b)
{
    return a.node == b.node && a.port == b.port && a.sourcePort == b.sourcePort;
}

inline bool operator!=(ConnectionDetail const& a, ConnectionDetail const& b) { return !(a == b); }

using DirectedAcyclicGraph = QHash<NodeId, dag::Entry>;

/// prints the graph as a mermaid flow chart useful for debugging
GT_INTELLI_EXPORT void debugGraph(DirectedAcyclicGraph const& graph);

} // namespace dag

using dag::DirectedAcyclicGraph;

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

    friend class GraphBuilder;

public:
    
    Q_INVOKABLE Graph();
    ~Graph();
    
    /**
     * @brief Returns a list of all nodes in this graph
     * @return
     */
    QList<Node*> nodes();
    QList<Node const*> nodes() const;
    
    /**
     * @brief Returns a list of all connections in this graph
     * @return
     */
    QList<Connection*> connections();
    QList<Connection const*> connections() const;
    
    /**
     * @brief Attempts to finde the node specified by the given nodeId
     * @param nodeId node id
     * @return node matched by nodeId (null if node was not found)
     */
    Node* findNode(NodeId nodeId);
    Node const* findNode(NodeId nodeId) const;

    dag::Entry* findNodeEntry(NodeId nodeId);
    dag::Entry const* findNodeEntry(NodeId nodeId) const;

    /**
     * @brief Attempts to finde a connection specified by the given connectionId
     * @param conId connection id
     * @return connection matched by conId (null if connection was not found)
     */
    Connection* findConnection(ConnectionId conId);
    Connection const* findConnection(ConnectionId conId) const;

    /**
     * @brief Fins all connections associated with the node specified by node id.
     * The connections can be narrowed down to ingoing and outgoing connection
     * @param nodeId node id
     * @param type Connection types
     * @return Connections
     */
    QVector<ConnectionId> findConnections(NodeId nodeId, PortType type = NoType) const;

    QVector<ConnectionId> findConnections(NodeId nodeId, PortType type, PortIndex idx) const;
    
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

    /**
     * @brief Returns the output provider of this graph. May be null if sub graph
     * was not yet initialized or it is the root graph.
     * @return Output provider
     */
    GroupOutputProvider* outputProvider();
    GroupOutputProvider const* outputProvider() const;

    /**
     * @brief Clears all nodes and connections
     */
    void clear();

    /**
     * @brief Appends the node to the intelli graph. Use this function instead
     * of appending the child directly. Node may change its id if its
     * already occupied
     * @param node Node to append
     * @param policy Whether to generate a new id if necessary
     * @return success
     */
    Node* appendNode(std::unique_ptr<Node> node, NodeIdPolicy policy = UpdateNodeId);

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

    [[deprecated]]
    void setNodePosition(Node* node, Position pos) {}

    DirectedAcyclicGraph const& dag() const { return m_nodes; }

signals:

    /**
     * @brief Emitted once a connection was appended
     * @param Pointer to connection object
     */
    void connectionAppended(Connection* connection);

    void connectionDeleted(ConnectionId connectionId);
    
    /**
     * @brief Emitted once a node was appended
     * @param Pointer to node object
     */
    void nodeAppended(Node* node);

    void nodeDeleted(NodeId nodeId);

    /**
     * @brief Emitted once the position of a node was altered. Is only triggered
     * by calling `setNodePosition`.
     * @param nodeId NodeId that changed the position
     * @param pos New position
     */
    void nodePositionChanged(NodeId nodeId, QPointF pos);

protected:
    
    bool triggerPortEvaluation(PortIndex idx = PortIndex{}) override;

    void onObjectDataMerged() override;

private:

    DirectedAcyclicGraph m_nodes;

    /**
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    ConnectionGroup& connectionGroup();
    ConnectionGroup const& connectionGroup() const;

    void restoreNode(Node* node);
    void restoreConnection(Connection* connection);

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders();

private slots:

    void forwardOutData(PortIndex idx);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPH_H
