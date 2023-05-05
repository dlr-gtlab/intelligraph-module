#include <QUuid>

#include "gt_igwireframenode.h"
#include "gt_colors.h"

GTIG_REGISTER_NODE(GtIgWireframeNode);

GtIgWireframeNode::GtIgWireframeNode() :
    GtIgAbstractShapeNode(tr("Wireframe"))
{

}

void
GtIgWireframeNode::compute(const QList<ShapePtr>& shapesIn,
                                QList<ShapePtr>& shapesOut)
{
    for (const ShapePtr& shape : shapesIn)
    {
        ShapePtr shapeCpy = shape->getCopy();
        shapeCpy->setTransparency(1.);
        shapeCpy->setOutlineStyle({1, gt::gui::color::text()});
        shapeCpy->setUuid(QUuid::createUuid().toString());
        shapesOut << shapeCpy;
    }
}
