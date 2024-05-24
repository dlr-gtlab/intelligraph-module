/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SCENE_H
#define GT_INTELLI_SCENE_H

#include <intelli/memory.h>
#include <intelli/graph.h>
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/connectionobject.h>

#include <gt_graphicsscene.h>

#include <map>


namespace intelli
{

class GraphAdapterModel;

class GraphScene : public GtGraphicsScene
{
    Q_OBJECT

public:

    using PortInfo = Node::PortInfo;
    using ConnectionShape = ConnectionGraphicsObject::ConnectionShape;

    GraphScene(Graph& graph);
    ~GraphScene();

    void reset();

    Graph& graph();
    Graph const& graph() const;

    /**
     * @brief Returns the scene data object, that is shared by all nodes and
     * grants access to scene specific properties.
     * @return Scene data.
     */
    GraphSceneData const& sceneData() const;

    /**
     * @brief Applies the new grid size. The grid size is used to snap the
     * movement of nodes in finite steps, thus aligning nodes to the grid if
     * the `snapToGrid` property is enabled.
     * @param gridSize Grid size used for snapping nodes
     */
    void setGridSize(double gridSize);

    /**
     * @brief Sets whether moving a node should always snap it to the grid
     * @param enable Whether to enable snap to grid
     */
    void setSnapToGrid(bool enable = true);

    NodeGraphicsObject* nodeObject(NodeId nodeId);
    NodeGraphicsObject const* nodeObject(NodeId nodeId) const;

    ConnectionGraphicsObject* connectionObject(ConnectionId conId);
    ConnectionGraphicsObject const* connectionObject(ConnectionId conId) const;

    QMenu* createSceneMenu(QPointF scenePos);

    void setConnectionShape(ConnectionGraphicsObject::ConnectionShape shape);

public slots:

    /**
     * @brief Alings all nodes to the grid
     */
    void alignObjectsToGrid();

    void deleteSelectedObjects();

    void duplicateSelectedObjects();

    bool copySelectedObjects();

    void pasteObjects();

protected:

    void keyPressEvent(QKeyEvent* event) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:

    struct Impl;

    struct NodeEntry
    {
        NodeId nodeId;
        volatile_ptr<NodeGraphicsObject, DirectDeleter> object;
    };

    struct ConnectionEntry
    {
        ConnectionId conId;
        volatile_ptr<ConnectionGraphicsObject, DirectDeleter> object;
    };

    /// graph this scene refers to
    QPointer<Graph> m_graph = nullptr;
    /// Node objects in this scene
    std::vector<NodeEntry> m_nodes;
    /// Connection objects in this scene
    std::vector<ConnectionEntry> m_connections;
    /// Draft connection if active
    volatile_ptr<ConnectionGraphicsObject> m_draftConnection;
    /// Shared scene data
    std::unique_ptr<GraphSceneData> m_sceneData;
    /// Shape style of the connections in this scene
    ConnectionShape m_connectionShape = ConnectionShape::DefaultShape;

    void beginReset();

    void endReset();
    
    void groupNodes(QVector<NodeGraphicsObject*> const& selectedNodeObjects);

    /**
     * @brief Updates the connection's end points. If a node graphics object
     * is passed in, only this side is updated.
     * @param object Connection object to update
     * @param node Node object that has changed (optional)
     */
    void moveConnection(ConnectionGraphicsObject* object,
                        NodeGraphicsObject* node = nullptr);

    /**
     * @brief Updates the connection's end point that the specified port type
     * refers to.
     * @param object Connection object to update
     * @param type Port type of the end point
     */
    void moveConnectionPoint(ConnectionGraphicsObject& object, PortType type);

    /**
     * @brief Highlights all nodes their ports that are compatible to `port`
     * @param node Source node
     * @param port Source port
     */
    void highlightCompatibleNodes(Node& node, PortInfo const& port);

    void clearHighlights();

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onNodeEvalStateChanged(QString nodeUuid);

    void onNodeShifted(NodeGraphicsObject* sender, QPointF diff);

    void onNodeMoved(NodeGraphicsObject* sender);

    void onConnectionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void moveConnections(NodeGraphicsObject* object);

    void onMakeDraftConnection(NodeGraphicsObject* object, ConnectionId conId);

    void onMakeDraftConnection(NodeGraphicsObject* object,
                               PortType type,
                               PortId portId);

    void onNodeContextMenu(NodeGraphicsObject* object, QPointF pos);

    void onPortContextMenu(NodeGraphicsObject* object, PortId portId, QPointF pos);
};

inline GraphScene*
nodeScene(QGraphicsObject& o)
{
    return qobject_cast<GraphScene*>(o.scene());
}

} // namespace intelli

#endif // GT_INTELLI_SCENE_H
