#ifndef NDSQWTBARCHARTMODEL_H
#define NDSQWTBARCHARTMODEL_H

#include "nds_abstractqwtmodel.h"

class NdsQwtBarChartModel : public NdsAbstractQwtModel
{
    Q_OBJECT

public:
    NdsQwtBarChartModel();

    ~NdsQwtBarChartModel() = default;

    QString caption() const override { return QString(" "); }

    QString name() const override { return QString("Qwt Bar Chart"); }

    virtual QString modelName() const { return QString("Qwt Bar Chart"); }

};

#endif // NDSQWTBARCHARTMODEL_H
