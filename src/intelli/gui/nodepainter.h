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
    enum PortRenderFlag : uint
    {
        NoPortFlag = 0,
        /// Whether the port is connected
        PortConnected = 1,
        /// Whether ports should be highlighted at all
        HighlightPorts = 2,
        /// Whether the port should be highlighted. Check `HighlightPorts` first
        PortHighlighted = 4,
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
     * @brief Applies pen and brush to the painter to render the outlne
     * of the node uniformly.
     * @param painter Painter to configure
     */
    void applyOutlineConfig(QPainter& painter) const;

    /**
     * @brief Returns the the background color of the node. Additional effects
     * may be applied. Override `customBackgroundColor` to apply a custom
     * background color similiar to how input/output provider and graphs have
     * an altered color.
     * @return Background color
     */
    QColor backgroundColor() const;

    /**
     * @brief Draws the background of the node.
     * @param painter Painter to draw with
     */
    virtual void drawBackground(QPainter& painter) const;

    /**
     * @brief Draws the outline of the node. Is repsonsible to highlight the
     * node when selecting or hovering. Use the predefined painter config.
     * @param painter Painter to draw with
     */
    virtual void drawOutline(QPainter& painter) const;

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
     * @param flags Port flag to draw the port according to it state
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
     * @param flags Port flag to draw the port according to it state
     */
    virtual void drawPortCaption(QPainter& painter,
                                 PortInfo const& port,
                                 PortType type,
                                 PortIndex idx,
                                 uint flags) const;

    /**
     * @brief Draws the resize handle
     * @param painter Painter to draw with
     */
    virtual void drawResizeHandle(QPainter& painter) const;

    void drawIcon(QPainter& painter) const;

    /**
     * @brief Draws the caption of the node
     * @param painter Painter to draw with
     */
    void drawCaption(QPainter& painter) const;

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
};

} // namespace intelli

#endif // GT_INTELLI_NODEPAINTER_H
