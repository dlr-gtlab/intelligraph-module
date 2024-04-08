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
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/connectionobject.h>

#include <map>

#include <QGraphicsScene>

namespace intelli
{

class GraphAdapterModel;

class GraphScene : public QGraphicsScene
{
    Q_OBJECT

public:

    using ConnectionShape = ConnectionGraphicsObject::ConnectionShape;

    GraphScene(Graph& graph);
    ~GraphScene();

    void reset();

    Graph& graph();
    Graph const& graph() const;

    NodeGraphicsObject* nodeObject(NodeId nodeId);
    NodeGraphicsObject const* nodeObject(NodeId nodeId) const;

    ConnectionGraphicsObject* connectionObject(ConnectionId conId);
    ConnectionGraphicsObject const* connectionObject(ConnectionId conId) const;

    QMenu* createSceneMenu(QPointF scenePos);

    void setConnectionShape(ConnectionGraphicsObject::ConnectionShape shape);

public slots:

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
    /// Shape style of the connections in this scene
    ConnectionShape m_connectionShape = ConnectionShape::DefaultShape;

    void beginReset();

    void endReset();
    
    void groupNodes(QVector<NodeGraphicsObject*> const& selectedNodeObjects);

    void moveConnection(ConnectionGraphicsObject* object, NodeGraphicsObject* node = nullptr);

    void moveConnectionPoint(ConnectionGraphicsObject* object, PortType type);

    void highlightCompatibleNodes(NodeId nodeId, PortType type, TypeId const& typeId);

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onNodeEvalStateChanged(NodeId nodeId);

    void onNodeShifted(NodeGraphicsObject* sender, QPointF diff);

    void onNodeMoved(NodeGraphicsObject* sender);

    void onConnectionAppended(Connection* con);

    void onConnectionDeleted(ConnectionId conId);

    void moveConnections(NodeGraphicsObject* object);

    void onMakeDraftConnection(NodeGraphicsObject* object, ConnectionId conId);

    void onMakeDraftConnection(NodeGraphicsObject* object, PortType type, PortId portId);

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
