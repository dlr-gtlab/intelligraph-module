/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHSCENE_H
#define GT_INTELLI_GRAPHSCENE_H

#include <intelli/memory.h>
#include <intelli/globals.h>
#include <intelli/gui/style.h>

#include <gt_graphicsscene.h>
#include <gt_command.h>

#include <QGraphicsObject>

class QMenu;

namespace intelli
{

class InteractableGraphicsObject;
class Node;
class NodeGraphicsObject;
class Graph;
class GraphSceneData;
class Connection;
class ConnectionGraphicsObject;
class CommentObject;
class CommentGraphicsObject;
class LineGraphicsObject;

class GraphScene : public GtGraphicsScene
{
    Q_OBJECT

public:

    GraphScene(Graph& graph);
    ~GraphScene();

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

    /// Returns whether snap to grid is enabled
    bool snapToGrid() const;

    void setConnectionShape(ConnectionShape shape);

    ConnectionShape connectionShape() const;

    NodeGraphicsObject* nodeObject(NodeId nodeId);
    NodeGraphicsObject const* nodeObject(NodeId nodeId) const;

    ConnectionGraphicsObject* connectionObject(ConnectionId conId);
    ConnectionGraphicsObject const* connectionObject(ConnectionId conId) const;

    std::unique_ptr<QMenu> createSceneMenu(QPointF scenePos);

public slots:

    /**
     * @brief Alings all nodes to the grid
     */
    void alignObjectsToGrid();

    void deleteSelectedObjects();

    void duplicateSelectedObjects();

    bool copySelectedObjects();

    void pasteObjects();

signals:

    void graphNodeDoubleClicked(Graph* graph);

    void connectionShapeChanged();

    void snapToGridChanged();

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
        unique_qptr<NodeGraphicsObject, DirectDeleter> object;
    };

    struct ConnectionEntry
    {
        ConnectionId conId;
        unique_qptr<ConnectionGraphicsObject, DirectDeleter> object;
    };

    struct CommentEntry
    {
        ObjectUuid uuid;
        unique_qptr<CommentGraphicsObject, DirectDeleter> object;
    };

    /// graph this scene refers to
    QPointer<Graph> m_graph;
    /// Node objects in this scene
    std::vector<NodeEntry> m_nodes;
    /// Connection objects in this scene
    std::vector<ConnectionEntry> m_connections;
    /// Comment objects in this scene
    std::vector<CommentEntry> m_comments;
    /// Draft connection if active
    unique_qptr<ConnectionGraphicsObject> m_draftConnection;
    unique_qptr<LineGraphicsObject> m_draftLine;
    /// Shared scene data
    std::unique_ptr<GraphSceneData> m_sceneData;
    /// Shape style of the connections in this scene
    ConnectionShape m_connectionShape = ConnectionShape::DefaultShape;
    /// Currently active command when moving nodes
    GtCommand m_nodeMoveCmd = {};

    /**
     * @brief Groups the selected nodes by moving the into a subgraph.
     * Instantiates ingoing and outgoing connections. Nodes and internal
     * connections are preserved.
     * @param selectedNodeObjects Nodes that should be grouped
     */
    void groupNodes(QVector<NodeGraphicsObject*> const& selectedNodeObjects);

    /**
     * @brief Expands the selected subgraph. Its nodes and internal
     * connections are preserved (except for the input/outputproviders).
     * Instantiates ingoing and outgoing connections.
     * @param groupNode Subgraph that should be expanded
     */
    void expandGroupNode(Graph* groupNode);

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

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    /// called while an object is being moved by the user
    void onObjectShifted(InteractableGraphicsObject* sender, QPointF diff);

    /// called if an object has been moved by the user (moving finished)
    void onObjectMoved(InteractableGraphicsObject* sender);

    /// called if node changed position externally
    void onNodePositionChanged(NodeGraphicsObject* sender);

    void onNodeDoubleClicked(NodeGraphicsObject* sender);

    void onConnectionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void moveConnections(NodeGraphicsObject* object);

    void onMakeDraftConnection(NodeGraphicsObject* object,
                               PortType type,
                               PortId portId);

    void onCommentAppended(CommentObject* comment);

    void onCommentDeleted(CommentObject* comment);

    void onNodeContextMenu(NodeGraphicsObject* object, QPointF pos);

    void onPortContextMenu(NodeGraphicsObject* object, PortId portId, QPointF pos);

    void onCommentContextMenu(CommentGraphicsObject* object, QPointF pos);
};

inline GraphScene*
nodeScene(QGraphicsObject& o)
{
    return qobject_cast<GraphScene*>(o.scene());
}

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENE_H
