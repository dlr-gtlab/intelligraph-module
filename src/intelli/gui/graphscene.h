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
     * @brief Alings all objects to the grid. If the scene contains a valid
     * selection, only the selected objects are aligned otherwise all objects
     * are aligned. Creates an undo-redo command.
     */
    void alignObjectsToGrid();

    /**
     * @brief Attempts to delete all selected object. If no objects are
     * selected, no objects will be deleted. Creates an undo-redo command.
     */
    void deleteSelectedObjects();

    /**
     * @brief Duplicates the selection. If no objects are selected, no action is
     * performed. Creates an undo-redo command.
     * TODO: Not applied to comments
     */
    void duplicateSelectedObjects();

    /**
     * @brief Copies the selection to the clipboard. If no objects are selected,
     * no action is performed.
     * TODO: Not applied to comments
     */
    bool copySelectedObjects();

    /**
     * @brief Pastes the selection from the clipboard. Creates an undo-redo
     * command.
     * TODO: Not applied to comments
     */
    void pasteObjects();

signals:

    void graphNodeDoubleClicked(Graph* graph);

    void connectionShapeChanged();

    void snapToGridChanged();

protected:

    void keyPressEvent(QKeyEvent* event) override;

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
    /// Shared scene data
    std::unique_ptr<GraphSceneData> m_sceneData;
    /// Shape style of the connections in this scene
    ConnectionShape m_connectionShape = ConnectionShape::DefaultShape;
    /// Currently active command when moving objects
    GtCommand m_objectMoveCmd = {};

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

private slots:

    /// called while an object is being moved by the user, inititates a move
    /// command if none has been started already
    void beginMoveCommand(InteractableGraphicsObject* sender, QPointF diff);

    /// called if an object has been moved by the user (moving finished),
    /// finalizes a move command if one is currently active
    void endMoveCommand(InteractableGraphicsObject* sender);

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onNodeDoubleClicked(NodeGraphicsObject* sender);

    void onConnectionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void onMakeDraftConnection(NodeGraphicsObject* object,
                               PortType type,
                               PortId portId);

    void onFinalizeDraftConnection(ConnectionId conId);

    void onCommentAppended(CommentObject* comment);

    void onCommentDeleted(CommentObject* comment);

    void onPortContextMenu(NodeGraphicsObject* object, PortId portId);

    void onObjectContextMenu(InteractableGraphicsObject* object);
};

inline GraphScene*
nodeScene(QGraphicsObject& o)
{
    return qobject_cast<GraphScene*>(o.scene());
}

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENE_H
