/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H
#define GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H

#include <intelli/exports.h>
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/graphicsobject.h>

namespace intelli
{

class Graph;

/**
 * @brief The InteractableGraphicsObject class.
 * Base class for all graph scene objects that should be moveable, resizeable,
 * collapsable, and that can recieve/react to hover events.
 * Hanldes translation and resizing uniformly.
 */
class GT_INTELLI_EXPORT InteractableGraphicsObject : public GraphicsObject
{
    Q_OBJECT

    using GraphicsObject::moveBy;

public:

    enum { Type = make_graphics_type<1>() };

    using InteractionFlags = size_t;

    enum InteractionFlag : InteractionFlags
    {
        NoInteractionFlag = 0,
        AllowTranslation  = 1 << 0,
        AllowResizing     = 1 << 1,
        DefaultInteractionFlags = AllowTranslation | AllowResizing
    };

    InteractableGraphicsObject(GraphSceneData const& data, QGraphicsItem* parent = nullptr);
    ~InteractableGraphicsObject();

    void setInteractionFlag(InteractionFlag flag, bool enable = true);

    QGraphicsObject* setupDropShadowEffect(std::function<QRectF()> boundingRectFunctor,
                                           std::function<void(QPainter&)> paintFunctor);

    size_t interactionFlags() { return m_flags; }

    /**
     * @brief Shifts (i.e. moves) the object by x and y respectively.
     * Object may not be shifted if the `AllowTranslation` flag is not set
     * @param x X difference
     * @param y Y difference
     */
    void shiftBy(double x, double y);

    /**
     * @brief Aligns the object to the grid
     */
    void alignToGrid();

    /**
     * @brief Returns the scene data object, that is shared by all nodes and
     * grants access to scene specific properties.
     * @return Scene data.
     */
    GraphSceneData const& sceneData() const;

    /**
     * @brief Returns whether this node is collapsed (node's body is hidden).
     * @return Is collapsed
     */
    bool isCollapsed() const;

    /**
     * @brief Sets the collapsed state of this objects (hides this object's body).
     * @param doCollapse Whether the object should be collapsed
     */
    void collapse(bool doCollapse = true);
    void setCollapsed(bool doCollapse = true);

    /**
     * @brief Returns the bounding rect of the main widget in scene-coordianates
     * May return an invalid rect if no widget is available
     * @return Scene bounding rect of the main widget
     */
    virtual QRectF widgetSceneBoundingRect() const;

    virtual ObjectUuid objectUuid() const = 0;

    /**
     * @brief Commits the position of this object to the associated node
     */
    virtual void commitPosition();

    /**
     * @brief Appends actions for the context menu
     * @param menu Menu
     */
    virtual void setupContextMenu(QMenu& menu);

protected:

    /// State of this object
    enum class State
    {
        Normal = 0,
        Translating,
        Resizing
    };

    /**
     * @brief Returns the current state
     * @return Current state
     */
    inline State state() const { return m_state; }

    /**
     * @brief Whether the object should start resizing.
     * @param localCoord Position of cursor within the graphics object.
     * Coordinates may be used to check if mouse hovers over a resize rect or
     * similar.
     * @return Whether the object should start resizing
     */
    virtual bool canResize(QPointF localCoord) = 0;

    /**
     * @brief Performs the resize action given the size difference.
     * @param diff Difference in size
     */
    virtual void resizeBy(QSize diff) = 0;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

signals:

    /**
     * @brief Emitted if the object was shifted (moved by x,y). The user is
     * still moving this object.
     * @param object Object that was shifted (this)
     * @param diff Difference that the object was shifted by
     */
    void objectShifted(InteractableGraphicsObject*, QPointF diff);

    /**
     * @brief Emitted once the object was moved to its "final" postion (i.e. the
     * user has stopped moving the object)
     * @param object Object that was moved (this)
     */
    void objectMoved(InteractableGraphicsObject*);

    /**
     * @brief Emitted once the object was collapsed or expanded.
     * @param object Object that was collapsed (this)
     * @param isCollapsed Whether the node was collapsed or expanded
     */
    void objectCollapsed(InteractableGraphicsObject*, bool isCollapsed);

    /**
     * @brief Emitted once the object was resized, either programmatically or
     * by the user. Should be called if "manually" resizing the object.
     */
    void objectResized(InteractableGraphicsObject*);

    /**
     * @brief Emitted once the context menu of an object was requested
     * @param object Object for which the context menu was requested (this)
     */
    void contextMenuRequested(InteractableGraphicsObject*);

private:

    /// Pointer to graph scene data
    GraphSceneData const* m_sceneData;
    /// Holds how much the node was shifted since the beginning of a
    /// translation operation
    QPointF m_translationStart;
    /// State flag
    State m_state = State::Normal;
    /// Interaction flags
    InteractionFlags m_flags = DefaultInteractionFlags;
    /// Whether the node is collapsed
    bool m_collapsed = false;
};

} // namespace intelli

#endif // GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H
