#ifndef NDSQMLLINECHARTMODEL_H
#define NDSQMLLINECHARTMODEL_H

#include "nds_helloqmlmodel.h"

class NdsQmlLineChartModel : public NdsHelloQmlModel
{
    Q_OBJECT

public:
    NdsQmlLineChartModel();

    ~NdsQmlLineChartModel() = default;

    QString caption() const override { return QString("Line Chart"); }

    QString name() const override { return QString("Line Chart"); }

    virtual QString modelName() const { return QString("Line Chart"); }

};

#endif // NDSQMLLINECHARTMODEL_H
