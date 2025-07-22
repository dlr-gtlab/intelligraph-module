/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPH_H
#define GT_INTELLI_GRAPH_H

#include <intelli/node.h>
#include <intelli/graphconnectionmodel.h>
#include <intelli/view.h>

#include <gt_finally.h>
#include <gt_platform.h>

class GtMdiItem;

namespace intelli
{

class Graph;
class GroupInputProvider;
class GroupOutputProvider;
class Connection;
class ConnectionGroup;
class DynamicNode;
class GraphExecutionModel;

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
GT_INTELLI_EXPORT
bool isAcyclic(Graph const& graph);

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

/// prints the graph as a mermaid flow chart useful for debugging
GT_INTELLI_EXPORT void debug(Graph const& graph);
GT_INTELLI_EXPORT void debug(ConnectionModel const& model);
GT_INTELLI_EXPORT void debug(GlobalConnectionModel const& model);

/**
 * @brief Returns whether the list of nodes contains a node with the id `nodeId`.
 * @tparam NodeId_t could be either NodeId or NodeUuid
 * @param nodeId Id of the node to search for
 * @param nodes Iterable list of nodes
 * @return Whether the list contains a node with the desired id
 */
template <typename NodeId_t, typename NodeList>
bool containsNodeId(NodeId_t const& nodeId, NodeList const& nodes)
{
    auto iter = std::find_if(nodes.begin(), nodes.end(), [&nodeId](auto node){
        return nodeId == get_node_id<NodeId_t>{}(node);
    });
    return iter != nodes.end();
}

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
     * @brief Returns the node id for the node given its object uuid. Returned
     * node id may be invalid if the object uuid does not belong to a node
     * belonging to this graph.
     * @param nodeUuid Node's oobject uuid
     * @return Node id (may be invalid)
     */
    NodeId nodeId(NodeUuid const& nodeUuid) const;

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
     * @brief Converts the connection uuid into a connection id (used for the
     * local connection model)
     * @param conUuid Connection uuid to convert
     * @return Connection id
     */
    ConnectionId connectionId(ConnectionUuid const&  conUuid) const;

    /**
     * @brief Converts the connection id into a connection uuid (used for the
     * global connection model)
     * @param conId Connection id to convert
     * @return Connection uuid
     */
    ConnectionUuid connectionUuid(ConnectionId conId) const;

    /**
     * @brief Access the graph of the given node
     * @param node Graph of the node
     * @return Graph
     */
    static Graph* accessGraph(Node& node);
    static Graph const* accessGraph(Node const& node);

    /**
     * @brief Returns the parent graph of this graph (null if the graph is a
     * root graph node)
     * @return Parent graph
     */
    Graph* parentGraph();
    Graph const* parentGraph() const;

    /**
     * @brief Returns the root graph of this graph. If no root is found it is
     * assumed that this object is the root.
     * @return Root graph (should never be null)
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
    [[deprecated("Use `connectionModel().iterateNodeIds()` instead.")]]
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
    [[deprecated]]
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
    [[deprecated("Use `connectionModel().iterateConnections(nodeId, type)` instead.")]]
    QVector<ConnectionId> findConnections(NodeId nodeId, PortType type = PortType::NoType) const;

    /**
     * @brief Overload. Finds all connections associated with the node and its
     * port id.
     * @param nodeId Node id
     * @param portId Port id
     * @return Connections
     */
    [[deprecated("Use `connectionModel().iterateConnections(nodeId, portId)` instead.")]]
    QVector<ConnectionId> findConnections(NodeId nodeId, PortId portId) const;

    /**
     * @brief Finds all connected nodes. I.e. ancestors or descendants. A node
     * is not listed twice.
     * @param nodeId Node id
     * @param type Connection filter (in/out going only or both)
     * @return Connections
     */
    [[deprecated("Use `connectionModel().iterateUniqueNodes(nodeId, type)` instead.")]]
    QVector<NodeId> findConnectedNodes(NodeId nodeId, PortType type = PortType::NoType) const;

    /**
     * @brief Overload. Finds all nodes that are connected to the source node
     * and port specified.
     * @param nodeId Node id
     * @param portId Port id
     * @return Connections
     */
    [[deprecated("Use `connectionModel().iterateUniqueNodes(nodeId, portId)` instead.")]]
    QVector<NodeId> findConnectedNodes(NodeId nodeId, PortId portId) const;

    /**
     * @brief Static method to that returns all target nodes without duplicates
     * @param connections Connections
     * @param type The type of connections, i.e. are these outgoing connections?
     * Then the target nodes are the ones on the in going/recieving side and
     * vice cersa.
     * @return List of all unique target nodes
     */
    [[deprecated]]
    static QVector<NodeId> uniqueTargetNodes(QVector<ConnectionId> const& connections, PortType type);

    /**
     * @brief Returns a list of all sub graphes (aka group nodes)
     * @return List of sub graphs
     */
    QList<Graph*> graphNodes();
    QList<Graph const*> graphNodes() const;
    
    /**
     * @brief Returns the input provider of this graph. May be null if the sub
     * graph was not yet initialized or it is the root graph.
     * @return Input provider
     */
    GroupInputProvider* inputProvider();
    GroupInputProvider const* inputProvider() const;

    /**
     * @brief Returns the input provider of this graph as a plain Node object.
     * @return Input provider
     */
    DynamicNode* inputNode();
    DynamicNode const* inputNode() const;

    /**
     * @brief Returns the output provider of this graph. May be null if the sub
     * graph was not yet initialized or it is the root graph.
     * @return Output provider
     */
    GroupOutputProvider* outputProvider();
    GroupOutputProvider const* outputProvider() const;

    /**
     * @brief Returns the output provider of this graph as a plain Node object.
     * @return Input provider
     */
    DynamicNode* outputNode();
    DynamicNode const* outputNode() const;

    /**
     * @brief Finds all dependencies of the node referred by `nodeId`
     * @param nodeId Node to find dependencies of
     * @return Dependencies
     */
    [[deprecated]]
    QVector<NodeId> findDependencies(NodeId nodeId) const;

    /**
     * @brief Finds all nodes that are depent of the node referred by `nodeId`
     * @param nodeId Node to find depent nodes of
     * @return Dependent nodes
     */
    [[deprecated]]
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
     * @return Node ptr
     */
    Node* appendNode(std::unique_ptr<Node> node,
                     NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Overload that accepts a unique ptr of type `T` and returns a
     * pointer of type `T`.
     * @param node Node to append
     * @param policy Whether to generate a new id if necessary
     * @return Node ptr of type `T`
     */
    template<typename T>
    inline T* appendNode(std::unique_ptr<T> node,
                         NodeIdPolicy policy = NodeIdPolicy::Update)
    {
        using Signature = Node*(Graph::*)(std::unique_ptr<Node>, NodeIdPolicy);

        // avoid recursive calls
        auto f  = static_cast<Signature>(&Graph::appendNode);
        Node* r = ((this->*f)(std::move(node), policy));
        return static_cast<T*>(r);
    }

    /**
     * @brief Appends a connection given by `conId`. Fails if the connection
     * already exists or the input and output ports were not found.
     * @param connection Connection to append
     * @return success
     */
    bool appendConnection(ConnectionId conId);

    /**
     * @brief Appends the connection to intelli graph. Use this function instead
     * of appending a connection object as a child directly. Aborts if the
     * connection already exists.
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
    [[deprecated("Use `moveNodes` instead.")]]
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
     * @brief Moves the node (given by NodeId) from this graph to the target
     * graph. Depending on the given `NodeIdPolicy` the node's id may be
     * updated. Failing to move a node will not restore the previous state
     * of the graph. Use a MementoDiff for this purpose. The underlying node
     * object is guranteed to be kept alive.
     * Note: All connections to this node are destroyed.
     * @param nodeId Node to move to other graph
     * @param policy Whether to update the node's id if necessary (if the
     * node id should be kept, moving the node may fail)
     * @return success
     */
    bool moveNode(NodeId nodeId,
                  Graph& targetGraph,
                  NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Overload that the node given by reference.
     * @param nodeId Node to move to other graph
     * @param policy Whether to update the node's id if necessary (if the
     * node id should be kept, moving the node may fail)
     * @return success
     */
    bool moveNode(Node& node,
                  Graph& targetGraph,
                  NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Moves the nodes from this graph to the target graph. The
     * connections inbetween these nodes are moved as well. Depending
     * on the given `NodeIdPolicy` the node's ids may be updated. Failing to
     * move a node may will not restore the previous state of the graph. The
     * underlying node objects are guranteed to be kept alive whereas the
     * connection objects are deleted. Thus, raw pointers to connection objects
     * must be updated.
     * @param nodes Nodes to move
     * @param targetGraph Target graph to move nodes to
     * @param policy Whether to generate a new id if necessary
     * @return success
     */
    bool moveNodesAndConnections(View<Node const*> nodes,
                                 Graph& targetGraph,
                                 NodeIdPolicy policy = NodeIdPolicy::Update);
    /// NodeId overload
    bool moveNodesAndConnections(View<NodeId> nodes,
                                 Graph& targetGraph,
                                 NodeIdPolicy policy = NodeIdPolicy::Update);
    /// QList overload
    bool moveNodesAndConnections(QList<Node const*> const& nodes,
                                 Graph& targetGraph,
                                 NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Same as `moveNodesAndConnections' except that no connection is
     * moved. These must be restored manually.
     * @param nodes Nodes to move
     * @param targetGraph Target graph to move nodes to
     * @param policy Whether to generate a new id if necessary
     * @return success
     */
    bool moveNodes(View<Node const*> nodes,
                   Graph& targetGraph,
                   NodeIdPolicy policy = NodeIdPolicy::Update);
    /// NodeId overload
    bool moveNodes(View<NodeId> nodes,
                   Graph& targetGraph,
                   NodeIdPolicy policy = NodeIdPolicy::Update);
    /// QList overload
    bool moveNodes(QList<Node const*> const& nodes,
                   Graph& targetGraph,
                   NodeIdPolicy policy = NodeIdPolicy::Update);

    /**
     * @brief Returns the local connection model.
     * @return Local connection model
     */
    ConnectionModel const& connectionModel() const;

    /**
     * @brief Returns the global connection model.
     * @return Global connection model
     */
    GlobalConnectionModel const& globalConnectionModel() const;

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders();

    /**
     * @brief Resets the global connection model. This might be necessary
     * because NodeUuids have changed.
     */
    void resetGlobalConnectionModel();

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
    void globalConnectionAppended(ConnectionUuid connectionUuid);

    /**
     * @brief Emitted after a conections was deleted
     * @param Connection id of the deleted connection
     */
    void connectionDeleted(ConnectionId connectionId);
    void globalConnectionDeleted(ConnectionUuid connectionUuid);

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

protected:

    void eval() override;

    void onObjectDataMerged() override;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    /**
     * @brief Whether this model is currently undergoing modification.
     * @return Is being modified
     */
    bool isBeingModified() const;

    /**
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    ConnectionGroup& connectionGroup();
    ConnectionGroup const& connectionGroup() const;

    bool appendNode(Node* node, NodeIdPolicy policy = NodeIdPolicy::Update);
    bool appendConnection(Connection* connection);

    void restoreNode(Node* node);
    void restoreConnection(Connection* connection);

    void restoreConnections();
    void restoreNodesAndConnections();

    void appendGlobalConnection(Connection* guard, ConnectionId conId, Node& targetNode);
    void appendGlobalConnection(Connection* guard, ConnectionUuid conUuid);

    void updateGlobalConnectionModel(std::shared_ptr<GlobalConnectionModel> const& ptr);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPH_H
