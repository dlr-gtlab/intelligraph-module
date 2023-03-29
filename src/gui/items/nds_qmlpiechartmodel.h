#ifndef NDSQMLPIECHARTMODEL_H
#define NDSQMLPIECHARTMODEL_H

#include "nds_helloqmlmodel.h"

class NdsQmlPieChartModel : public NdsHelloQmlModel
{
    Q_OBJECT

public:
    NdsQmlPieChartModel();

    ~NdsQmlPieChartModel() = default;

    QString caption() const override { return QString("Pie Chart"); }

    QString name() const override { return QString("Pie Chart"); }

    virtual QString modelName() const { return QString("Pie Chart"); }

};

#endif // NDSQMLPIECHARTMODEL_H
