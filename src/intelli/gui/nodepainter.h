/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 26.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEPAINTER_H
#define GT_INTELLI_NODEPAINTER_H

#include <intelli/node.h>

class QColor;
class QPainter;
class QGraphicsItem;

namespace intelli
{

class NodeGeometry;
class NodeGraphicsObject;

class GT_INTELLI_EXPORT NodePainter
{
public:

    using PortData = Node::PortData;

    NodePainter(NodeGraphicsObject& object, NodeGeometry& geometry);
    NodePainter(NodePainter const&) = delete;
    NodePainter(NodePainter&&) = delete;
    NodePainter& operator=(NodePainter const&) = delete;
    NodePainter& operator=(NodePainter&&) = delete;
    virtual ~NodePainter() = default;

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

    virtual QColor backgroundColor() const;

    virtual void drawBackground(QPainter& painter) const;

    virtual void drawOutline(QPainter& painter) const;

    void drawPorts(QPainter& painter) const;

    virtual void drawPort(QPainter& painter,
                          PortData& port,
                          PortType type,
                          PortIndex idx,
                          bool connected) const;

    virtual void drawPortCaption(QPainter& painter,
                                 PortData& port,
                                 PortType type,
                                 PortIndex idx,
                                 bool connected) const;

    void drawResizeHandle(QPainter& painter) const;

    void drawCaption(QPainter& painter) const;

    void paint(QPainter& painter) const;

protected:

    NodeGraphicsObject& object() const;
    Node& node() const;
    NodeGeometry& geometry() const;

private:
    NodeGraphicsObject* m_object;
    NodeGeometry* m_geometry;
};

} // namespace intelli

#endif // GT_INTELLI_NODEPAINTER_H
