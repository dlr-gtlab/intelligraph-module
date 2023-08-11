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

    friend class ModelAdapter;
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
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    ConnectionGroup& connectionGroup();
    ConnectionGroup const& connectionGroup() const;
    
    /**
     * @brief Attempts to finde node specified by the given nodeId
     * @param nodeId node id
     * @return node matched by nodeId (null if node was not found)
     */
    Node* findNode(NodeId nodeId);
    Node const* findNode(NodeId nodeId) const;

    /**
     * @brief Attempts to finde a connection specified by the given connectionId
     * @param conId connection id
     * @return connection matched by conId (null if connection was not found)
     */
    Connection* findConnection(ConnectionId const& conId);
    Connection const* findConnection(ConnectionId const& conId) const;
    
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

    /**
     * @brief Updates the position of the node associated with nodeId. Prefer
     * this over setting the position directly, as the changes may not be
     * forwarded to the graph model.
     * @param nodeId Node to update
     * @param pos New position
     */
    void setNodePosition(Node* node, QPointF pos);

    /**
     * @brief Returns the graph model adapter (may be null)
     * @return Graph model
     */
    ModelAdapter* findModelAdapter();
    ModelAdapter const* findModelAdapter() const;

    /**
     * @brief Creates a graph model adapter if it does not exists already.
     * It is uesd to evaluate nodes
     * @param policy Inidctaes whether the instance should be considered an
     * active or a dummy model
     * @return Graph model adapter
     */
    ModelAdapter* makeModelAdapter(ModelPolicy policy = ActiveModel);

    /**
     * @brief Clears the graph model adapter thus stopping the evaluation of all
     * nodes. Should be called once the graph model is no longer used.
     * @param force Force to close the model regardless of its model policy
     */
    void clearModelAdapter(bool force = true);

signals:

    /**
     * @brief Emitted once a connection was appended
     * @param Pointer to connection object
     */
    void connectionAppended(Connection* connection);
    
    /**
     * @brief Emitted once a node was appended
     * @param Pointer to node object
     */
    void nodeAppended(Node* node);

    /**
     * @brief Emitted once the position of a node was altered. Is only triggered
     * by calling `setNodePosition`.
     * @param nodeId NodeId that changed the position
     * @param pos New position
     */
    void nodePositionChanged(NodeId nodeId, QPointF pos);

protected:

    // keep graph model up date if a node or connection was restored
    void onObjectDataMerged() override;

private:

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders();

private slots:

    void forwardInData(PortIndex idx);
    void forwardOutData(PortIndex idx);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPH_H
