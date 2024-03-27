/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 27.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/graphics/nodecaptionsobject.h"
#include "intelli/gui/nodegeometry.h"
#include "intelli/gui/nodepainter.h"

using namespace intelli;

NodeCaptionGraphicsObject::NodeCaptionGraphicsObject(QGraphicsObject& parent,
                                                     NodeGeometry& geometry,
                                                     NodePainter& painter) :
    QGraphicsObject(&parent),
    m_geometry(&geometry),
    m_painter(&painter)
{
    setZValue(1);
}

QRectF
NodeCaptionGraphicsObject::boundingRect() const
{
    return QRectF{QPointF{0, 0}, m_geometry->captionRect().size()};
}

void
NodeCaptionGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsObject::mousePressEvent(event);
}

void
NodeCaptionGraphicsObject::paint(QPainter* painter,
                                 QStyleOptionGraphicsItem const* option,
                                 QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    assert(painter);

    m_painter->drawCaption(*painter, *this);
}
