#ifndef NDSSHAPECOLORMODEL_H
#define NDSSHAPECOLORMODEL_H

#include <QColor>

#include "nds_abstractshapemodel.h"

class NdsShapeColorModel : public NdsAbstractShapeModel
{
    Q_OBJECT

public:
    NdsShapeColorModel();


    ~NdsShapeColorModel() = default;

    QString caption() const override { return QString("Shape Color"); }

    QString name() const override { return QString("Shape Color"); }

    virtual QString modelName() const { return QString("ShapeColor"); }

    QWidget* embeddedWidget() override;

    void compute(const QList<ShapePtr>& shapesIn,
                 QList<ShapePtr>& shapesOut) override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QWidget* m_mainWid;

    QColor m_color;

    void setWidgetColor();

};

#endif // NDSSHAPECOLORMODEL_H
