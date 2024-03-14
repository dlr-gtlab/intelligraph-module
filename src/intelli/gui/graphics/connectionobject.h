/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H
#define GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H

#include <intelli/connection.h>

#include <QGraphicsObject>
#include <QPointer>

namespace intelli
{

class ConnectionGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    ConnectionGraphicsObject(Connection& connection);

    QRectF boundingRect() const override;

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

private:

    QPointer<Connection> m_connection;
};

} // namespace intelli

#endif // CONNECTIONGRAPHICSOBJECT_H
