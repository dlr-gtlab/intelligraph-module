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

#include <QtNodes/Definitions>

#include <QPointer>

class GtObjectGroup;

namespace QtNodes { struct ConnectionId; }

namespace intelli
{

class GroupInputProvider;
class GroupOutputProvider;
class Connection;
class ConnectionGroup;
class ModelAdapter;

class Graph : public Node
{
    Q_OBJECT
    
    friend class GroupInputProvider;
    friend class GroupOutputProvider;

    template <intelli::PortType>
    friend class AbstractGroupProvider;

    using ConnectionId = QtNodes::ConnectionId;

public:
    
    Q_INVOKABLE Graph();
    ~Graph();
    
    QList<Node*> nodes();
    QList<Node const*> nodes() const;
    
    QList<Connection*> connections();
    QList<Connection const*> connections() const;

    /**
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    ConnectionGroup& connectionGroup();
    ConnectionGroup const& connectionGroup() const;
    
    Node* findNode(NodeId nodeId);
    Node const* findNode(NodeId nodeId) const;
    
    Connection* findConnection(ConnectionId const& conId);
    Connection const* findConnection(ConnectionId const& conId) const;
    
    QList<Graph*> subGraphs();
    QList<Graph const*> subGraphs() const;
    
    GroupInputProvider* inputProvider();
    GroupInputProvider const* inputProvider() const;
    
    GroupOutputProvider* outputProvider();
    GroupOutputProvider const* outputProvider() const;

    /**
     * @brief Returns the graph model adapter (may be null)
     * @return Graph model
     */
    ModelAdapter* findModelAdapter();
    ModelAdapter const* findModelAdapter() const;

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
     * @brief Updates the position of the node associated with nodeId.
     * @param nodeId Node to update
     * @param pos New position
     */
    void setNodePosition(Node* node, QPointF pos);

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

    /**
     * @brief initGroupProviders
     */
    void initGroupProviders();

protected:

    // keep graph model up date if a node or connection was restored
    void onObjectDataMerged() override;

    NodeDataPtr eval(PortId outId) override;

signals:
    
    void connectionAppended(Connection* connection);
    
    void nodeAppended(Node* node);

    void nodePositionChanged(NodeId nodeId, QPointF pos);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPH_H
