#include <QUuid>

#include "nds_wireframemodel.h"

NdsWireframeModel::NdsWireframeModel()
{

}

void NdsWireframeModel::compute(const QList<ShapePtr>& shapesIn,
                                QList<ShapePtr>& shapesOut)
{
    foreach (const ShapePtr shape, shapesIn)
    {
        ShapePtr shapeCpy = shape->getCopy();
        shapeCpy->setTransparency(1.);
        shapeCpy->setOutlineStyle({1, QColor("white")});
        shapeCpy->setUuid(QUuid::createUuid().toString());
        shapesOut << shapeCpy;
    }
}
