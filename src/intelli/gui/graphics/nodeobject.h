/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEGRAPHICSOBJECT_H

#include <intelli/globals.h>
#include <intelli/gui/graphics/interactableobject.h>

#include <QPointer>

class QGraphicsProxyWidget;

namespace intelli
{

class Node;
class NodeUI;
class NodeUIData;
class NodePainter;
class NodeGeometry;
class NodeEvalStateGraphicsObject;
struct GraphSceneData;

/**
 * @brief Graphic object representing a node in the graph.
 * This class is responsible for handling the selection when clicked,
 * starting a draft connection or resizing the node if it has a widget.
 * The rendering is denoted by the Painter and Geometry objects.
 */
class GT_INTELLI_EXPORT NodeGraphicsObject : public InteractableGraphicsObject
{
    Q_OBJECT

    class NodeProxyWidget;

public:

    class Highlights;

    // Needed for graphics_cast
    enum { Type = make_graphics_type<GraphicsItemType::Node, InteractableGraphicsObject>() };
    int type() const override { return Type; }

    /**
     * @brief constructor
     * @param scene
     * @param data Graph scene data which holds shared data for all node objects
     * @param node Node that this graphic object represents
     * @param ui Node UI used to access painter and geomtery data
     */
    NodeGraphicsObject(QGraphicsScene& scene, GraphSceneData& data, Node& node, NodeUI& ui);
    ~NodeGraphicsObject();

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

    ObjectUuid objectUuid() const override;

    /**
     * @brief Returns the node's ui data object. Used for painting the node.
     * @return Ui Data
     */
    NodeUIData const& uiData() const;

    /**
     * @brief Whether the resize handle should be displayed
     * @return Has resize handle
     */
    bool hasResizeHandle() const;

    DeletableFlag deletableFlag() const override;

    DeleteOrdering deleteOrdering() const override;

    bool deleteObject() override;

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

    /**
     * @brief Returns the bounding rect of the main widget in scene-coordianates
     * May return an invalid rect if no widget is available
     * @return Scene bounding rect of the main widget
     */
    QRectF widgetSceneBoundingRect() const override;

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
     * @brief Commits the position of this object to the associated node
     */
    void commitPosition() override;

    /**
     * @brief (Re-) embedds the main widget of this graphics object
     */
    void embedCentralWidget();

    /**
     * @brief Appends actions for the context menu
     * @param menu Menu
     */
    void setupContextMenu(QMenu& menu) override;

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

public slots:

    /**
     * @brief Updates the visuals of the node once the underlying node object
     * has changed.
     */
    void refreshVisuals();

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    /**
     * @brief Whether the object should start resizing.
     * @param localCoord Position of cursor within the graphics object.
     * Coordinates may be used to check if mouse hovers over a resize rect or
     * similar.
     * @return Whether the object should start resizing
     */
    bool canResize(QPointF localCoord) override;

    /**
     * @brief Performs the resize action given the size difference.
     * @param diff Difference in size
     */
    void resizeBy(QSize diff) override;

signals:

    /**
     * @brief Emitted once a draft connection should be started at the given
     * port. Its up to the scene to implement and handle the draft connection.
     * @param object Object that triggered the draft connection (this)
     * @param type Port type
     * @param port Port to start the connection at
     */
    void makeDraftConnection(NodeGraphicsObject* object, PortType type, PortId port);

    /**
     * @brief Emitted once a node's position was updated externally (i.e. NOT
     * by moving the node's graphics objec)
     * @param object Object that updated its position (this)
     */
    void nodePositionChanged(NodeGraphicsObject* object);

    /**
     * @brief Emitted once the node was double clicked
     * @param object Object that was double clicked (this)
     */
    void nodeDoubleClicked(NodeGraphicsObject* object);

    /**
     * @brief Emitted once the node's changed its geometry and was updated.
     * @param object Object that was double clicked (this)
     */
    void nodeGeometryChanged(NodeGraphicsObject* object);

    /**
     * @brief Emitted once the context menu of a port was requested
     * @param object Object for which the port's context menu was requested (this)
     * @param port Port for which the context menu should be requested
     */
    void portContextMenuRequested(NodeGraphicsObject* object, PortId port);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

private slots:

    /**
     * @brief Updates the visuals of the node once the position of the
     * underlying node object has changed.
     */
    void onNodePositionChanged();

    /**
     * @brief Helper method to update all child items once the object's geometry
     * changed
     */
    void updateChildItems();
};

} // namespace intelli

#endif // GT_INTELLI_NODEGRAPHICSOBJECT_H
