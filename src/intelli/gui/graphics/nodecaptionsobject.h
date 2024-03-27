/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 27.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODECAPTIONGRAPHICSOBJECT_H
#define GT_INTELLI_NODECAPTIONGRAPHICSOBJECT_H

#include <QGraphicsObject>

namespace intelli
{

class NodeGeometry;
class NodePainter;

class NodeCaptionGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    NodeCaptionGraphicsObject(QGraphicsObject& parent, NodeGeometry& geometry, NodePainter& painter);

    QRectF boundingRect() const override;

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

private:

    NodeGeometry* m_geometry;
    NodePainter* m_painter;
};

} // namespace intelli

#endif // GT_INTELLI_NODECAPTIONGRAPHICSOBJECT_H
