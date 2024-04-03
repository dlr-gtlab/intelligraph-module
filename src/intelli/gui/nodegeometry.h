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
#include <QPointer>
#include <QWidget>

namespace intelli
{

class Node;
class GT_INTELLI_EXPORT NodeGeometry
{
public:

    explicit NodeGeometry(Node& node);
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

    void setWidget(QPointer<QWidget> widget);

    int hspacing() const;
    int vspacing() const;

    QPainterPath shape() const;

    QRectF innerRect() const;

    QRectF boundingRect() const;

    virtual QRectF captionRect() const;

    virtual QRectF evalStateRect() const;

    virtual QPointF widgetPosition() const;

    virtual QRectF portRect(PortType type, PortIndex idx) const;

    virtual QRectF portCaptionRect(PortType type, PortIndex idx) const;

    PortHit portHit(QPointF coord) const;
    PortHit portHit(QRectF coord) const;

    virtual QRectF resizeHandleRect() const;

    /// tells the geometry to invalidate the cached geometry
    void recomputeGeomtry();

protected:

    Node& node() const;

    QWidget const* widget() const;

    virtual QPainterPath computeShape() const;

    virtual QRectF computeInnerRect() const;

    virtual QRectF computeBoundingRect() const;

private:

    Node* m_node;

    QPointer<QWidget> m_widget;
    // cache for inner rect
    mutable tl::optional<QRectF> m_innerRect;
    // cache for bounding rect
    mutable tl::optional<QRectF> m_boundingRect;
    // cache for shape
    mutable tl::optional<QPainterPath> m_shape;

    bool positionWidgetAtBottom() const;

    int captionHeightExtend() const;

    int portHorizontalExtent(PortType type) const;

    int portHeightExtent() const;
};

} // namespace intelli

#endif // GT_INTELLI_NODEGEOMETRY_H
