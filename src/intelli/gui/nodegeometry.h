/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEGEOMETRY_H
#define GT_INTELLI_NODEGEOMETRY_H

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <thirdparty/tl/optional.hpp>

#include <QPainterPath>
#include <QPointer>
#include <QWidget>

namespace intelli
{

class Node;

/**
 * @brief The NodeGeometry class.
 * Defines how the node graphics object is structured. I.e. where are the ports
 * placed, the caption, the eval state visualizer, the resize handle etc.
 *
 * This class implements the default implementation for nodes. It should be
 * subclassed to override this default implementation. Use `intelli::style` for
 * predefined sizes of certain graphical components, such as the port size.
 *
 * To calculate where all components are places in respect to each other, use
 * the `innerRect`. This rect is soely used to organize the components. If the
 * `computeBoundingRect` or `computeShape` methods are not overriden, it is also
 * used for the bounding rect and the shape.
 *
 * To invalidate the cached geometry data use `recomputeGeometry`
 */
class GT_INTELLI_EXPORT NodeGeometry
{
public:

    friend class NodeGraphicsObject;

    /**
     * @brief Constructor.
     * @param node The node for which the grapical layout should be computed
     */
    explicit NodeGeometry(Node const& node);
    NodeGeometry(NodeGeometry const&) = delete;
    NodeGeometry(NodeGeometry&&) = delete;
    NodeGeometry& operator=(NodeGeometry const&) = delete;
    NodeGeometry& operator=(NodeGeometry&&) = delete;
    virtual ~NodeGeometry() = default;

    /// helper struct to check whether a port was hit
    struct PortHit
    {
        PortType type{PortType::NoType};
        PortId port{};

        operator bool() const
        {
            return type != PortType::NoType && port != invalid<PortId>();
        }
    };

    /**
     * @brief Predefined horizontal spacing between ports and central widget
     * @return Horizontal spacing
     */
    int hspacing() const;
    /**
     * @brief Predefined vertical spacing between ports and other components
     * @return Vertical spacing
     */
    int vspacing() const;

    /**
     * @brief Returns the shape, which is used for the collision detection of
     * the actual graphics object. It should be accurate but not too complex.
     * The shape is cached, since it can be costly to compute. When computing
     * the shape it is safe to call `shape`, however during the execution of
     * `computeShape` this function returns an empty `QPainterPath`.
     * Defaults to `boundingRect`.
     * @return Shape
     */
    QPainterPath const& shape() const;

    /**
     * @brief Returns the inner rect of the graphics object. This rect
     * is soely intended to organize how the components (ports, caption etc.) are
     * organized in respect to each other. The shape is cached, since it can be
     * costly to compute. When computing the inner rect it is safe to call
     * `innerRect`, however during the execution of `computeInnerRect` this
     * function returns an empty `QRectF`.
     * @return Inner rect
     */
    QRectF innerRect() const;

    /**
     * @brief Returns the bounding rect of the graphics object. Within this rect
     * all subsequent painting must happen. The rect is cached, since it can be
     * costly to compute. When computing the bounding rect it is safe to call
     * `boundingRect`, however during the execution of `computeBoundingRect`
     * this function returns an empty `QRectF`.
     * Defaults to `innerRect` with some additional margin.
     * @return Bounding rect
     */
    QRectF boundingRect() const;

    /**
     * @brief Denotes where the caption should be placed. The width is exactly
     * the width that is required for rendering the caption text. When
     * overriding it is advised to call the base implementation and to simply
     * move the rect to the target location.
     * @return Caption rect
     */
    virtual QRectF captionRect() const;

    /**
     * @brief Denotes where the eval state should be placed. When overriding it
     * is advised to call the base implementation and to simply
     * move the rect to the target location.
     * @return Eval state rect
     */
    virtual QRectF evalStateRect() const;

    /**
     * @brief Defines where the widget is positioned. If there is no widget this
     * function should return a default constructed point. To access the width
     * and height of the wiget use `widget()`.
     * @return Position of the widget
     */
    virtual QPointF widgetPosition() const;

    /**
     * @brief Denotes where the port (only the port and not the caption) is
     * placed. Remeber to respect port properties when overriding, e.g. whether
     * the port is visible.
     * @param type Port type
     * @param idx Port index
     * @return Port rect
     */
    virtual QRectF portRect(PortType type, PortIndex idx) const;

    /**
     * @brief Denotes where the port caption is placed. Remeber to respect port
     *  properties when overriding, e.g. whether the port caption is visible.
     * @param type Port type
     * @param idx Port index
     * @return Port caption rect
     */
    virtual QRectF portCaptionRect(PortType type, PortIndex idx) const;

    /**
     * @brief Check if the point is within a port rect.
     * @param coord Point to check
     * @return Port hit helper struct
     */
    PortHit portHit(QPointF coord) const;
    /**
     * @brief Check if the rect is intersects with a port rect.
     * @param rect Rect to check
     * @return Port hit helper struct
     */
    PortHit portHit(QRectF rect) const;

    /**
     * @brief Denotes where the resize handle should be positioned.
     * @return Resize handle rect
     */
    virtual QRectF resizeHandleRect() const;

    /**
     * @brief Tells the geometry to invalidate the cached geometry once it is
     * required. (Lazy evaluation)
     */
    void recomputeGeomtry();

protected:

    /**
     * @brief Returns the associated node
     * @return Node
     */
    Node const& node() const;

    /**
     * @brief Returns the associated widget. May be null.
     * @return Widget
     */
    QWidget const* centralWidget() const;

    /**
     * @brief Override this method to define a custom collision shape
     * @return Shape
     */
    virtual QPainterPath computeShape() const;

    /**
     * @brief Override this method to define a custom inner rect used to
     * organize the components (ports, caption etc.)
     * @return Inner rect
     */
    virtual QRectF computeInnerRect() const;

    /**
     * @brief Override this method to define a custom bounding rect. This must
     * big enough to accomodate all subsequent painting calls.
     * @return Bounding rect
     */
    virtual QRectF computeBoundingRect() const;

private:

    /// Node
    Node const* m_node;
    /// Widget
    QPointer<QWidget const> m_widget;
    /// cache for inner rect
    mutable tl::optional<QRectF> m_innerRect;
    /// cache for bounding rect
    mutable tl::optional<QRectF> m_boundingRect;
    /// cache for shape
    mutable tl::optional<QPainterPath> m_shape;

    bool positionWidgetAtBottom() const;

    int captionHeightExtend() const;

    int portHorizontalExtent(PortType type) const;

    int portHeightExtent() const;

    void setWidget(QPointer<QWidget const> widget);
};

} // namespace intelli

#endif // GT_INTELLI_NODEGEOMETRY_H
