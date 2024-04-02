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

class NodePainter
{
public:

    NodePainter(NodeGraphicsObject& obj, NodeGeometry& geometry);
    NodePainter(NodePainter const&) = delete;
    NodePainter(NodePainter&&) = delete;
    NodePainter& operator=(NodePainter const&) = delete;
    NodePainter& operator=(NodePainter&&) = delete;
    virtual ~NodePainter() = default;

    virtual QColor backgroundColor() const;

    virtual void drawBackground(QPainter& painter);

    virtual void drawOutline(QPainter& painter);

    void drawPorts(QPainter& painter);

    virtual void drawPort(QPainter& painter, Node::PortData& port, PortType type, PortIndex idx, bool connected);

    virtual void drawPortCaption(QPainter& painter, Node::PortData& port, PortType type, PortIndex idx, bool connected);

    void drawResizeHandle(QPainter& painter);

    void drawCaption(QPainter& painter);

    void paint(QPainter& painter);

private:
    NodeGraphicsObject* m_object;
    NodeGeometry* m_geometry;
};

} // namespace intelli

#endif // GT_INTELLI_NODEPAINTER_H
