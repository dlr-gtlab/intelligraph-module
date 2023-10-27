/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_MODELADAPTER_H
#define GT_INTELLI_MODELADAPTER_H

#include <intelli/graph.h>

#include <QtNodes/AbstractGraphModel>
#include <QSize>
#include <QPointer>

namespace intelli
{

class Graph;
class Node;
class Connection;

class GraphAdapterModel : public QtNodes::AbstractGraphModel
{
    Q_OBJECT

public:

    GraphAdapterModel(Graph& graph);
    ~GraphAdapterModel();
    
    Graph& graph();
    Graph const& graph() const;

    QtNodes::ConnectionId convert(ConnectionId conId) const;
    ConnectionId          convert(QtNodes::ConnectionId conId) const;

    /**
     * @brief Generates a new unique NodeId.
     * @return
     */
    QtNodes::NodeId newNodeId() override;

    /**
    * @brief Returns the full set of unique Node Ids.
    * Model creator is responsible for generating unique `unsigned int`
    * Ids for all the nodes in the graph. From an Id it should be
    * possible to trace back to the model's internal representation of
    * the node.
    */
    std::unordered_set<QtNodes::NodeId> allNodeIds() const override;

    /**
     * @returns `true` if there is data in the model associated with the
     * given `nodeId`.
     */
    bool nodeExists(QtNodes::NodeId nodeId) const  override;

    /**
     * A collection of all input and output connections for the given `nodeId`.
     */
    std::unordered_set<QtNodes::ConnectionId>
    allConnectionIds(QtNodes::NodeId nodeId) const override;

    /**
     * @brief Returns all connected Node Ids for given port.
     * The returned set of nodes and port indices correspond to the type
     * opposite to the given `portType`.
     */
    std::unordered_set<QtNodes::ConnectionId>
    connections(QtNodes::NodeId nodeId,
                QtNodes::PortType portType,
                QtNodes::PortIndex index) const override;

    /**
     * @brief Checks if two nodes with the given `connectionId` are connected.
     */
    bool connectionExists(QtNodes::ConnectionId connectionId) const override;

    /**
     * @brief Creates a new node instance in the derived class.
     * The model is responsible for generating a unique `NodeId`.
     * @param[in] nodeType is free to be used and interpreted by the
     * model on its own, it helps to distinguish between possible node
     * types and create a correct instance inside.
     */
    QtNodes::NodeId addNode(QString const& nodeType = {}) override;

    /**
     * @brief Creates a new connection between two nodes.
     * Default implementation emits signal
     * `connectionCreated(connectionId)`
     *
     * In the derived classes user must emite the signal to notify the
     * scene about the changes.
     */
    void addConnection(QtNodes::ConnectionId connectionId) override;

    /**
     * @brief Model decides if a conection with a given connection Id possible.
     * The default implementation compares corresponding data types.
     *
     * It is possible to override the function and connect non-equal
     * data types.
     */
    bool connectionPossible(QtNodes::ConnectionId connectionId) const override;

    /**
     * @brief Returns node-related data for requested NodeRole.
     * @returns Node Caption, Node Caption Visibility, Node Position etc.
     */
    QVariant nodeData(QtNodes::NodeId nodeId,
                      QtNodes::NodeRole role) const  override;

    /**
     * @brief Sets node properties.
     * Sets: Node Caption, Node Caption Visibility, Style, State, Node Position etc.
     * @see NodeRole.
     */
    bool setNodeData(QtNodes::NodeId nodeId,
                     QtNodes::NodeRole role,
                     QVariant value) override;

    QtNodes::NodeFlags nodeFlags(QtNodes::NodeId nodeId) const override;

    QtNodes::NodeEvalState nodeEvalState(QtNodes::NodeId nodeId) const override;

    /**
     * @brief Returns port-related data for requested NodeRole.
     * @returns Port Data Type, Port Data, Connection Policy, Port
     * Caption.
     */
    QVariant portData(QtNodes::NodeId nodeId,
                      QtNodes::PortType portType,
                      QtNodes::PortIndex index,
                      QtNodes::PortRole role) const override;

    bool setPortData(QtNodes:: NodeId nodeId,
                     QtNodes::PortType portType,
                     QtNodes::PortIndex index,
                     QVariant const &value,
                     QtNodes::PortRole role = QtNodes::PortRole::Data) override;

    bool deleteConnection(QtNodes::ConnectionId connectionId) override;

    bool deleteNode(QtNodes::NodeId const nodeId) override;

public slots:

    /**
     * @brief Commit the final position to the node object. SHould be called
     * after a node was moved
     * @param Node id
     */
    void commitPosition(NodeId nodeId);

private slots:

    void onNodeEvalStateUpdated();

    void onNodeChanged();

    void onPortAboutToBeInserted(PortType type, PortIndex idx);
    void onPortInserted(PortType type, PortIndex idx);

    void onPortAboutToBeDeleted(PortType type, PortIndex idx);
    void onPortDeleted(PortType type, PortIndex idx);

private:

    struct Geometry
    {
        Position pos;
        QSize size;
    };

    QHash<NodeId, Geometry> m_geometries;

    QPointer<Graph> m_graph;

    QVector<ConnectionId> m_shiftedConnections;
};

} // namespace intelli;

#endif // GT_INTELLI_MODELADAPTER_H
