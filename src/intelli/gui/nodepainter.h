/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEPAINTER_H
#define GT_INTELLI_NODEPAINTER_H

#include <intelli/node.h>

class QColor;
class QPainter;
class QGraphicsItem;

namespace intelli
{

class NodeUIData;
class NodeGeometry;
class NodeGraphicsObject;

/**
 * @brief The NodePainter class.
 * Denotes how the node graphic object should be rendered.
 *
 * This class implements the default implementation for nodes. It should be
 * subclassed to override this default implementation. Use `intelli::style` for
 * predefined sizes and colors of certain graphical components, such as the
 * port size.
 */
class GT_INTELLI_EXPORT NodePainter
{
public:

    using PortInfo = Node::PortInfo;
    using PortData [[deprecated("Use PortInfo")]] = PortInfo;

    /// Flags to tell the painter the state of the port
    enum RenderFlag : uint
    {
        /// No Render Flag
        NoPortFlag [[deprecated("use `NoRenderFlag` instead")]] = 0,
        NoRenderFlag = 0,

        /* General Render Flags */

        /// Whether the current QPainter config should be used as is
        /// (by default it may be overriden)
        UsePainterConfig = 1 << 0,

        /* Node Background Specific Render Flags */

        /// Whether the background should be rendered
        DrawNodeBackground = 1 << 1,
        /// Whether the outline should be rendered
        DrawNodeOutline= 1 << 2,
        /// Whether the resize hanlde should be drawn
        DrawNodeResizeHandle = 1 << 3,

        DefaultNodeRenderFlags = DrawNodeBackground |
                                 DrawNodeOutline |
                                 DrawNodeResizeHandle,

        /* Port Specific Render Flags */

        /// Whether the port is connected
        PortConnected = 1 << 5,
        /// Whether ports should be highlighted at all
        PortHighlightsActive = 1 << 6,
        HighlightPorts [[deprecated("use `PortHighlightsActive` instead")]] = PortHighlightsActive,
        /// Whether the port should be highlighted. Check `HighlightPorts` first
        PortHighlighted = 1 << 7,
        /// Default render flags for ports
        DefaultPortRenderFlags = NoRenderFlag
    };

    /**
     * @brief Constructor
     * @param object Graphic object to paint
     * @param geometry Geometry to use to draw components at the correct position
     */
    NodePainter(NodeGraphicsObject const& object,
                NodeGeometry const& geometry);
    NodePainter(NodePainter const&) = delete;
    NodePainter(NodePainter&&) = delete;
    NodePainter& operator=(NodePainter const&) = delete;
    NodePainter& operator=(NodePainter&&) = delete;
    virtual ~NodePainter();

    /**
     * @brief Applies pen and brush to the painter to render the background
     * of the node uniformly.
     * @param painter Painter to configure
     */
    void applyBackgroundConfig(QPainter& painter) const;

    /**
     * @brief Applies pen and brush to the painter to render the outline
     * of the node uniformly.
     * @param painter Painter to configure
     */
    void applyOutlineConfig(QPainter& painter) const;

    /**
     * @brief Applies pen and brush to the painter to render the drop shadow
     * effect of the node uniformly.
     * @param painter Painter to configure
     */
    void applyDropShadowConfig(QPainter& painter) const;

    /**
     * @brief Applies pen and brush to the painter to render a port based on
     * the port properties uniformly.
     * of the node uniformly.
     * @param painter Painter to configure
     */
    void applyPortConfig(QPainter& painter,
                         PortInfo const& port,
                         PortType type,
                         PortIndex idx,
                         uint flags) const;

    /**
     * @brief Returns the the background color of the node. Additional effects
     * may be applied. Override `customBackgroundColor` to apply a custom
     * background color similiar to how input/output provider and graphs have
     * an altered color.
     * @return Background color
     */
    QColor backgroundColor() const;

    /**
     * @brief Returns the icon that should be displayed in the node's header.
     * This function respects the collapsed-state of the node and may override
     * `uiData().displayIcon()`.
     * @return Whether a display icon should be drawn
     */
    QIcon displayIcon() const;

    /**
     * @brief Draws the background and outline of the node. Will also call
     * `drawResizeHandle`.
     * @param painter Painter to draw with
     * @param flags Render flags. Ignores port render flags.
     */
    virtual void drawBackground(QPainter& painter, uint flags = DefaultNodeRenderFlags) const;

    /**
     * @brief Calls `drawPort` for each port that is visible and
     * `drawPortCaption` if the port caption is visible.
     * @param painter Painter to draw with
     */
    void drawPorts(QPainter& painter) const;

    /**
     * @brief Draws the connection point of the port
     * @param painter Painter to draw with
     * @param port Port info
     * @param type Port type
     * @param idx Port index
     * @param flags Port flag to draw the port according to it state. Ignores
     * node render flags by defualt.
     */
    virtual void drawPort(QPainter& painter,
                          PortInfo const& port,
                          PortType type,
                          PortIndex idx,
                          uint flags) const;

    /**
     * @brief Draws the caption of the port
     * @param painter Painter to draw with
     * @param port Port info
     * @param type Port type
     * @param idx Port index
     * @param flags Port flag to draw the port according to it state. Ignores
     * node render flags by defualt.
     */
    virtual void drawPortCaption(QPainter& painter,
                                 PortInfo const& port,
                                 PortType type,
                                 PortIndex idx,
                                 uint flags) const;

    /**
     * @brief Draws the resize handle
     * @param painter Painter to draw with
     * @param flags Render flags. Ignores port and node render flags.
     */
    virtual void drawResizeHandle(QPainter& painter, uint flags = NoRenderFlag) const;

    /**
     * @brief Draws the caption of the node
     * @param painter Painter to draw with
     * @param flags Render flags. Ignores port and node render flags.
     */
    void drawCaption(QPainter& painter, uint flags = NoRenderFlag) const;

    /**
     * @brief Draws the `displayIcon`.
     * @param painter Painter to draw with
     */
    void drawIcon(QPainter& painter) const;

    /**
     * @brief Main paint method, used to draw all components in the right
     * order.
     * @param painter Painter to draw with
     */
    void paint(QPainter& painter) const;

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
     * @brief Returns the associated node.
     * @return Node
     */
    Node const& node() const;

    /**
     * @brief Returns the geometry used for the organization of all components
     * @return Geometry
     */
    NodeGeometry const& geometry() const;

    /**
     * @brief May be overriden to apply a custom background color similiar
     * to how input/output provider and graphs have an altered color.
     * @return Background color
     */
    virtual QColor customBackgroundColor() const;

private:

    /// Graphic object
    NodeGraphicsObject const* m_object;
    /// Geometry
    NodeGeometry const* m_geometry;
    /// padding
    alignas(8) uint8_t __padding[16];

    void drawBackgroundHelper(QPainter& painter) const;
};

} // namespace intelli

#endif // GT_INTELLI_NODEPAINTER_H
