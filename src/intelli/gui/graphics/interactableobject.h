/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H
#define GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H

#include <intelli/exports.h>
#include <intelli/gui/graphscenedata.h>

#include <QGraphicsObject>

namespace intelli
{

/**
 * @brief The InteractableGraphicsObject class.
 * Base class for all graph scene objects that should be moveable, resizeable,
 * collapsable, and that can recieve/react to hover events.
 * Implements translation and resizing in a uniformly.
 */
class GT_INTELLI_EXPORT InteractableGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    InteractableGraphicsObject(GraphSceneData const& data, QGraphicsObject* parent = nullptr);
    ~InteractableGraphicsObject();

    /**
     * @brief Returns the scene data object, that is shared by all nodes and
     * grants access to scene specific properties.
     * @return Scene data.
     */
    inline GraphSceneData const& sceneData() const { return *m_sceneData; }

    /**
     * @brief Returns whether this node is currently hovered (via the cursor).
     * @return Is hovered
     */
    inline bool isHovered() const { return m_hovered; }

    /**
     * @brief Returns whether this node is collapsed (node's body is hidden).
     * @return Is collapsed
     */
    inline bool isCollapsed() const { return m_collapsed; }

    /**
     * @brief Sets the collapsed state of this objects (hides this object's body).
     * @param doCollapse Whether the object should be collapsed
     */
    void collapse(bool doCollapse = true);
    void setCollapse(bool doCollapse = true);

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
    virtual void resize(QSize diff) = 0;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

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

private:

    /// Pointer to graph scene data
    GraphSceneData const* m_sceneData;
    /// Holds how much the node was shifted since the beginning of a
    /// translation operation
    QPointF m_translationStart;
    /// State flag
    State m_state = State::Normal;
    /// Whether node is hovered
    bool m_hovered = false;
    /// Whether the node is collapsed
    bool m_collapsed = false;
};

} // namespace intelli

#endif // GT_INTELLI_INTERACTABLEGRAPHICSOBJECT_H
