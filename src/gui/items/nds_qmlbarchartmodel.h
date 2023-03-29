#ifndef NDSQMLBARCHARTMODEL_H
#define NDSQMLBARCHARTMODEL_H

#include "nds_helloqmlmodel.h"

class NdsQmlBarChartModel : public NdsHelloQmlModel
{
    Q_OBJECT

public:
    NdsQmlBarChartModel();

    ~NdsQmlBarChartModel() = default;

    QString caption() const override { return QString("Bar Chart"); }

    QString name() const override { return QString("Bar Chart"); }

    virtual QString modelName() const { return QString("Bar Chart"); }

};

#endif // NDSQMLBARCHARTMODEL_H
