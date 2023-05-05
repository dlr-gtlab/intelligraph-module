#ifndef NDS3DPLOT_H
#define NDS3DPLOT_H

#include "gtl_shape.h"

#include "gt_mdiitem.h"

class Scene3d;
class View3d;

class Nds3DPlot : public GtMdiItem
{
    Q_OBJECT

public:
    Q_INVOKABLE Nds3DPlot();

    void setData(GtObject* obj) override;

    void addShapes(QList<ShapePtr>& shapes);

private:
    /// 3D scene widget
    Scene3d* m_3dscene;

    /// 3D view widget
    View3d* m_3dview;

private slots:

    void onInitialized();
};

#endif // NDS3DPLOT_H
