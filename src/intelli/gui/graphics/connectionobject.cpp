/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/graphics/connectionobject.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

using namespace intelli;

ConnectionGraphicsObject::ConnectionGraphicsObject(Connection& connection) :
    m_connection(&connection)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    setZValue(-1.0);
}

QRectF
ConnectionGraphicsObject::boundingRect() const
{
    return {};
}

void
ConnectionGraphicsObject::paint(QPainter* painter,
                                QStyleOptionGraphicsItem const* option,
                                QWidget* widget)
{
    painter->setClipRect(option->exposedRect);
}

