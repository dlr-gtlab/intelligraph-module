/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEGRAPHICSOBJECT_H

#include <intelli/node.h>
#include <intelli/nodedatainterface.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/gui/nodepainter.h>

#include <QPointer>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>

namespace intelli
{

class NodeUI;
class NodeEvalStateGraphicsObject;

/**
 * @brief Graphic object representing a node in the graph.
 * This class is responsible for handling the selection when clicked,
 * starting a draft connection or resizing the node if it has a widget.
 * The rendering is denoted by the Painter and Geometry objects.
 */
class GT_INTELLI_EXPORT NodeGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Node };
    int type() const override { return Type; }

    /**
     * @brief constructor
     * @param graph Graph to which the node belongs
     * @param node Node that this graphic object represents
     * @param ui Node UI used to access painter and geomtery data
     */
    NodeGraphicsObject(Graph& graph, Node& node, NodeUI& ui);

    /**
     * @brief Returns the associated node
     * @return Node
     */
    Node& node();
    Node const& node() const;

    /**
     * @brief Returns the associated node id.
     * @return Node id
     */
    NodeId nodeId() const;

    /**
     * @brief Returns the associated graph.
     * @return Graph
     */
    Graph& graph();
    Graph const& graph() const;

    /**
     * @brief Returns whether this node is currently hovered (via the cursor).
     * @return Is hovered
     */
    bool isHovered() const;

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

    /**
     * @brief Shape for collision detection
     * @return Shape
     */
    QPainterPath shape() const override;

    /**
     * @brief Returns the central widget object
     * @return
     */
    QGraphicsWidget* centralWidget();
    QGraphicsWidget const* centralWidget() const;

    /**
     * @brief Returns the geometry object, denoting the layout of the graphics
     * object.
     * @return Geometry
     */
    NodeGeometry const& geometry() const;

    /**
     * @brief (Re-) embedds the main widget of this graphics object
     */
    void embedCentralWidget();

    /**
     * @brief Commits the position of this object to the associated node
     */
    void commitPosition();

    /**
     * @brief Sets the node eval state to visualize the current evaluation
     * state of the node to the user
     * @param state Evaluation state
     */
    void setNodeEvalState(NodeEvalState state);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

signals:

    void makeDraftConnection(NodeGraphicsObject* object, ConnectionId conId);

    void makeDraftConnection(NodeGraphicsObject* object, PortType type, PortId port);

    void nodeShifted(NodeGraphicsObject* object, QPointF diff);

    void nodeMoved(NodeGraphicsObject* object);

    void nodeGeometryChanged(NodeGraphicsObject* object);

    void portContextMenuRequested(NodeGraphicsObject* object, PortId port, QPointF pos);

    void contextMenuRequested(NodeGraphicsObject* object, QPointF pos);

private:

    struct Impl;

    enum State
    {
        Normal = 0,
        Translating,
        Resizing
    };

    /// graph
    QPointer<Graph> m_graph;
    /// Associated node
    QPointer<Node> m_node;
    /// Geometry
    std::unique_ptr<NodeGeometry> m_geometry;
    /// Painter
    std::unique_ptr<NodePainter> m_painter;
    /// Central widget
    QPointer<QGraphicsProxyWidget> m_proxyWidget = nullptr;
    /// Node eval state object
    NodeEvalStateGraphicsObject* m_evalStateObject = nullptr;

    // flags
    State m_state = Normal;
    bool m_hovered = false;

private slots:

    /**
     * @brief Updates the visuals of the node
     */
    void onNodeChanged();

    /**
     * @brief Helper method to update all child items
     */
    void updateChildItems();
};

} // namespace intelli

#endif // GT_INTELLI_NODEGRAPHICSOBJECT_H
