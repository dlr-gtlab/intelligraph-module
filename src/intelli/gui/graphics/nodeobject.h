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
struct GraphSceneData;

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

    class Highlights;

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Node };
    int type() const override { return Type; }

    /**
     * @brief constructor
     * @param data Graph scene data which holds shared data for all node objects
     * @param graph Graph to which the node belongs
     * @param node Node that this graphic object represents
     * @param ui Node UI used to access painter and geomtery data
     */
    NodeGraphicsObject(GraphSceneData& data,
                       Graph& graph,
                       Node& node,
                       NodeUI const& ui);

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
     * @brief Returns the scene data object, that is shared by all nodes and
     * grants access to scene specific properties.
     * @return Scene data.
     */
    GraphSceneData const& sceneData() const;

    /**
     * @brief Returns whether this node is currently hovered (via the cursor).
     * @return Is hovered
     */
    bool isHovered() const;

    /**
     * @brief Whether the resize handle should be displayed
     * @return Has resize handle
     */
    bool hasResizeHandle() const;

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
     * @brief Returns a helper object, that contains all highlight specific data
     * @return Highlights objects.
     */
    Highlights& highlights();
    Highlights const& highlights() const;

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

    /**
     * @brief The Highlights class.
     * Denotes whether a node or a port should be highlighted by the painter.
     * This is only intended for visualizations.
     */
    class Highlights
    {
        friend class NodeGraphicsObject;

        explicit Highlights(NodeGraphicsObject& object);

        NodeGraphicsObject* m_object = nullptr;
        /// List of highlightes ports
        /// (used preallocated array as a preliminary optimization)
        QVarLengthArray<PortId, 10> m_compatiblePorts;
        /// Whether ports should be highlighted
        bool m_isActive = false;
        /// Whether the node is considered compatible
        bool m_isNodeCompatible = false;

    public:

        Highlights(Highlights const&) = delete;
        Highlights(Highlights&&) = delete;
        Highlights& operator=(Highlights const&) = delete;
        Highlights& operator=(Highlights&&) = delete;
        ~Highlights() = default;

        /**
         * @brief Returns whether highlighting is active
         * @return Is active
         */
        bool isActive() const;
        /**
         * @brief Whether this node is should be marked as compatible
         * @return Is node comaptible
         */
        bool isNodeCompatible() const;
        /**
         * @brief Returns whether the port should be marked compatible
         * @param port Port to check
         * @return Is port highlighted
         */
        bool isPortCompatible(PortId port) const;
        /**
         * @brief Highlights this node and all ports as incompatible
         */
        void setAsIncompatible();
        /**
         * @brief Highlights all ports that are compatible to the TypeId.
         * @param typeId TypeId to check
         * @param type Which ports to check
         */
        void setCompatiblePorts(TypeId const& typeId, PortType type);
        /**
         * @brief Forces the port to be marked as compatible (visual only). Does
         * not override other highlights.
         * @param port Port to mark as compatible
         */
        void setPortAsCompatible(PortId port);
        /**
         * @brief Clears the highlights.
         */
        void clear();
    };

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

    /**
     * @brief Emitted if the node was shifted (moved by x,y).
     * @param object Object that was moved
     * @param diff Difference that the object was moved by
     */
    void nodeShifted(NodeGraphicsObject* object, QPointF diff);

    /**
     * @brief Emitted once the node was moved to its "final" postion (i.e. the
     * user no longer has ended the move operation)
     * @param object Object that was moved
     */
    void nodeMoved(NodeGraphicsObject* object);

    void nodeGeometryChanged(NodeGraphicsObject* object);

    void portContextMenuRequested(NodeGraphicsObject* object, PortId port, QPointF pos);

    void contextMenuRequested(NodeGraphicsObject* object, QPointF pos);

private:

    struct Impl;
    class NodeProxyWidget;

    enum State
    {
        Normal = 0,
        Translating,
        Resizing
    };

    /// Pointer to graph scene data
    GraphSceneData* m_sceneData;
    /// Pointer to graph
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
    /// Holds how much the node was shifted since the beginning of a
    /// translation operation
    QPointF m_translationDiff;
    /// Highlight data
    Highlights m_highlights;
    /// State flag
    State m_state = Normal;
    /// Whether node is hovered
    bool m_hovered = false;

    /**
     * @brief selects the item and the node in the application
     */
    void selectNode();

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
