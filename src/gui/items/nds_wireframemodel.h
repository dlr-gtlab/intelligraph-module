#ifndef NDSWIREFRAMEMODEL_H
#define NDSWIREFRAMEMODEL_H

#include "nds_abstractshapemodel.h"

class NdsWireframeModel : public NdsAbstractShapeModel
{
    Q_OBJECT

public:
    NdsWireframeModel();

    ~NdsWireframeModel() = default;

    QString caption() const override { return QString("Wireframe"); }

    QString name() const override { return QString("Shape Wireframe"); }

    virtual QString modelName() const { return QString("ShapeWireframe"); }

    void compute(const QList<ShapePtr>& shapesIn,
                 QList<ShapePtr>& shapesOut) override;

};

#endif // NDSWIREFRAMEMODEL_H
