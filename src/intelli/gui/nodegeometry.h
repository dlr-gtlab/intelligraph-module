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

#include <QGraphicsWidget>
#include <QPainterPath>
#include <QPointer>

namespace intelli
{

class Node;
class NodeUIData;
class NodeGraphicsObject;

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
    explicit NodeGeometry(NodeGraphicsObject const& object);
    NodeGeometry(NodeGeometry const&) = delete;
    NodeGeometry(NodeGeometry&&) = delete;
    NodeGeometry& operator=(NodeGeometry const&) = delete;
    NodeGeometry& operator=(NodeGeometry&&) = delete;
    virtual ~NodeGeometry();

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
     * @brief Returns whether the node should draw a display icon.
     * This function respects collapsed-state of the node and may override
     * `uiData().hasDisplayIcon()`.
     * @return Whether a display icon should be drawn
     */
    bool hasDisplayIcon() const;

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
     * @brief Returns the bounding rect of the graphics object. Within this rect
     * all subsequent painting must happen. The rect is cached, since it can be
     * costly to compute. When computing the bounding rect it is safe to call
     * this function, however during the execution of `computeBoundingRect`
     * this function returns an empty `QRectF`.
     *
     * Note: Defaults to `nodeHeaderRect` united with `nodeBodyRect` with some
     * additional margin.
     * @return Bounding rect
     */
    QRectF boundingRect() const;

    /**
     * @brief Returns the header rect of the node. By default the caption, eval
     * state, and icon is placed. The rect is cached, since it may be costly to
     * compute and is often requested multiple times to place other components.
     * When computing the header rect it is safe to call this function, however
     * during the execution of `computeNodeHeaderRect` this function returns an
     * empty `QRectF`.
     * @return header rect
     */
    QRectF nodeHeaderRect() const;

    /**
     * @brief Returns the body rect of the node. By default the ports, widget,
     * and reesize rect is placed within this rect. The rect is cached, since
     * it may be costly to compute and is often requested multiple times to
     * place other components. When computing the body rect it is safe to call
     * this function, however during the execution of `computeNodeBodyRect` this
     * function returns an empty `QRectF`.
     * @return body rect
     */
    QRectF nodeBodyRect() const;

    /**
     * @brief Denotes where the caption should be placed (i.e. rendered). When
     * overriding it is advised to call the base implementation and to simply
     * move the rect to the target location.
     * @return Caption rect
     */
    virtual QRectF captionRect() const;

    /**
     * @brief Returns the size of the caption. The size is required for
     * rendering the caption text (single line).
     * @return Size of caption
     */
    QSize captionSize() const;

    /**
     * @brief Denotes where the icon should be placed.
     * @return Size of caption
     */
    virtual QRect iconRect() const;
    /**
     * @brief Returns the size of the icon. This function is provided for
     * convenience and simply forwards the size set in the current style.
     * @return Size of icon
     */
    QSize iconSize() const;

    /**
     * @brief Denotes where the eval state should be placed. When overriding it
     * is advised to call the base implementation and to simply
     * move the rect to the target location.
     * @return Eval state rect
     */
    virtual QRectF evalStateRect() const;

    /**
     * @brief Returns the size of the eval state object. This function is
     * provided for convenience and simply forwards the size set in the current
     * style.
     * @return Size of eval state object
     */
    QSize evalStateSize() const;

    /**
     * @brief Defines where the widget is positioned. If there is no widget this
     * function should return a default constructed point. To access the width
     * and height of the wiget use `widget()`.
     * @return Position of the widget
     */
    virtual QPointF widgetPosition() const;

    /**
     * @brief Returns the size of the widget.
     * @return Size of the widget ((0, 0) if no widget exists)
     */
    QSize widgetSize() const;

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
     * @brief Tells the geometry to invalidate cached geometry.
     */
    void recomputeGeometry();

protected:

    /**
     * @brief Returns the associated graphic object.
     * @return Graphic object
     */
    NodeUIData const& uiData() const;

    /**
     * @brief Returns the associated graphic object.
     * @return Graphic object
     */
    NodeGraphicsObject const& object() const;

    /**
     * @brief Returns the associated node
     * @return Node
     */
    Node const& node() const;

    /**
     * @brief Returns the associated widget. May be null.
     * @return Widget
     */
    QGraphicsWidget const* centralWidget() const;

    /**
     * @brief Override this method to define a custom collision shape
     * @return Shape
     */
    virtual QPainterPath computeShape() const;

    /**
     * @brief Override this method to define a custom header rect used to
     * organize header components (caption, eval statee, icon)
     * @return Inner rect
     */
    virtual QRectF computeNodeBodyRect() const;

    /**
     * @brief Override this method to define a custom body rect used to
     * organize main components (ports, widgets, resize rect)
     * @return Inner rect
     */
    virtual QRectF computeNodeHeaderRect() const;

    /**
     * @brief Override this method to define a custom bounding rect. This must
     * big enough to accomodate all subsequent painting calls.
     * @return Bounding rect
     */
    virtual QRectF computeBoundingRect() const;

    int portHorizontalExtent(PortType type) const;

    int portVerticalExtent() const;

private:

    /// Graphic object
    NodeGraphicsObject const* m_object;
    /// Widget
    QPointer<QGraphicsWidget const> m_widget;
    /// cache for header rect
    mutable tl::optional<QRectF> m_headerRect;
    /// cache for body rect
    mutable tl::optional<QRectF> m_bodyRect;
    /// cache for bounding rect
    mutable tl::optional<QRectF> m_boundingRect;
    /// cache for shape
    mutable tl::optional<QPainterPath> m_shape;
    /// padding
    alignas(8) uint8_t __padding[24];

    /**
     * @brief Returns whether to position the node below all ports
     * @return Whether to position the node below all ports
     */
    bool positionWidgetBelowPorts() const;

    /**
     * @brief Update the widget pointer.
     * @param widget Widget
     */
    void setWidget(QPointer<QGraphicsWidget const> widget);
};

} // namespace intelli

#endif // GT_INTELLI_NODEGEOMETRY_H
