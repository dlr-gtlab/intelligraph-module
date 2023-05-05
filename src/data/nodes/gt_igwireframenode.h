#ifndef GT_IGWIREFRAMENODE_H
#define GT_IGWIREFRAMENODE_H

#include "gt_igabstractshapenode.h"

class GtIgWireframeNode : public GtIgAbstractShapeNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgWireframeNode();

protected:

    void compute(const QList<ShapePtr>& shapesIn,
                 QList<ShapePtr>& shapesOut) override;
};

#endif // GT_IGWIREFRAMENODE_H
