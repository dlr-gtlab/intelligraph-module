#include <QVBoxLayout>

#include "gt_application.h"

#include "gtl_scene3d.h"
#include "gtl_view3d.h"

#include "nds_3dplot.h"

Nds3DPlot::Nds3DPlot()
{
    setObjectName("3D Plot");

    m_3dview = new View3d(nullptr);

    m_3dscene = new Scene3d();

    QVBoxLayout* lay = new QVBoxLayout;

    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    widget()->setLayout(lay);

    lay->addWidget(m_3dview);

    connect(m_3dview, SIGNAL(initialized()), this, SLOT(onInitialized()));

    m_3dview->setScene(m_3dscene);
}

void
Nds3DPlot::setData(GtObject *obj)
{

}

void
Nds3DPlot::addShapes(QList<ShapePtr>& shapes)
{
    foreach (ShapePtr shape, shapes)
    {

        m_3dscene->updateShape(shape);
    }

    m_3dview->fitAll();
}

void Nds3DPlot::onInitialized()
{
    m_3dview->viewAxo();

    m_3dscene->setFaceBoundariesEnabled(true);
    m_3dscene->setFaceBoundariesLineWidth(0.5);

    QColor white(36, 49, 63);
    m_3dview->setAxisCrossTextColor(1);

    if (!gtApp->inDarkMode())
    {
        white = QColor(255, 255, 255);
        m_3dview->setAxisCrossTextColor(0);
    }


    m_3dview->setBackgroundGradient(white, white);
//    m_3dview->setAxisCrossEnabled(true);
}
