/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 26.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEGEOMETRY_H
#define GT_INTELLI_NODEGEOMETRY_H

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <thirdparty/tl/optional.hpp>

#include <QPainterPath>

namespace intelli
{

class Node;
class NodeGeometry
{
public:

    NodeGeometry(Node& node);
    NodeGeometry(NodeGeometry const&) = delete;
    NodeGeometry(NodeGeometry&&) = delete;
    NodeGeometry& operator=(NodeGeometry const&) = delete;
    NodeGeometry& operator=(NodeGeometry&&) = delete;
    virtual ~NodeGeometry() = default;

    struct PortHit
    {
        PortType type{PortType::NoType};
        PortId port{};

        operator bool() const
        {
            return type != PortType::NoType && port != invalid<PortId>();
        }
    };

    bool positionWidgetAtBottom() const;

    int hspacing() const;
    int vspacing() const;

    QPainterPath shape() const;

    QRectF innerRect() const;

    QRectF boundingRect() const;

    QRectF captionRect() const;

    QRectF evalStateRect() const;

    QPointF widgetPosition() const;

    QRectF portRect(PortType type, PortIndex idx) const;

    QRectF portCaptionRect(PortType type, PortIndex idx) const;

    PortHit portHit(QPointF coord) const;
    PortHit portHit(QRectF coord) const;

    QRectF resizeHandleRect() const;

    /// tells the geometry to recompute the geometry
    void recomputeGeomtry();

private:

    Node* m_node;
    // cache for inner rect
    mutable tl::optional<QRectF> m_innerRect;

    int captionHeightExtend() const;

    int portHorizontalExtent(PortType type) const;

    int portHeightExtent() const;
};

} // namespace intelli

#endif // GT_INTELLI_NODEGEOMETRY_H
