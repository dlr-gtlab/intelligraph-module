#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QEvent>
#include <QBasicTimer>

#include "gt_mdilauncher.h"
#include "gt_mdiitem.h"
#include "gt_application.h"

#include "gtl_scene3d.h"
#include "gtl_view3d.h"

#include "nds_shapedata.h"
#include "nds_3dplot.h"

#include "nds_shapevisualizationmodel.h"

class NdsLabel : public QLabel
{
public:
    NdsLabel(const QString& str, NdsShapeVisualizationModel* model) :
        QLabel(str), m_model(model)
    {

    }

    void mouseDoubleClickEvent(QMouseEvent *event)
    {
//        gtInfo() << "Double clicked!";
        m_model->open3DPlot();
    }

    void mousePressEvent(QMouseEvent *event)
    {
//        if (timer.isActive()) {
//            timer.stop();
//            gtInfo() << "double click";
//        }
//        else {
//            timer.start(500, this);
//        }
    }

//    void timerEvent(QTimerEvent *) {
//        timer.stop();
//        gtInfo() << "single click";
//    }

//    QBasicTimer timer;

    NdsShapeVisualizationModel* m_model;
};

NdsShapeVisualizationModel::NdsShapeVisualizationModel() :
    m_canvas(new QWidget),
    _label(new NdsLabel("Shape Viewer", this))
{

    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _label->setMinimumSize(200, 200);

    if (gtApp->inDarkMode())
    {
        _label->setStyleSheet("QLabel { background-color : rgb(36, 49, 63); color : blue; }");
    }
    else
    {
        _label->setStyleSheet("QLabel { background-color : rgb(255, 255, 255); color : blue; }");
    }
    _label->installEventFilter(this);

    m_3dview = new View3d(m_canvas);

    QVBoxLayout* lay = new QVBoxLayout;
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    m_canvas->setLayout(lay);

//    lay->addWidget(m_3dview);
//    m_3dview->hide();
    lay->addWidget(_label);

//    QSizePolicy sizePolicy;
//    sizePolicy.setVerticalPolicy(QSizePolicy::Expanding);
//    sizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
//    m_3dview->setSizePolicy(sizePolicy);

    m_3dscene = new Scene3d();

    connect(m_3dview, SIGNAL(initialized()), this, SLOT(onInitialized()));

    m_3dview->setScene(m_3dscene);
}

QWidget*
NdsShapeVisualizationModel::embeddedWidget()
{
    return m_canvas;
    //    return m_3dview;
}

bool
NdsShapeVisualizationModel::eventFilter(QObject *object, QEvent *event)
{
    if (object == _label) {
        int w = _label->width();
        int h = _label->height();

        if (event->type() == QEvent::Resize) {
            _label->setPixmap(m_pixmap.scaled(w, h, Qt::KeepAspectRatio));
        }
//        else if (event->type() == QEvent::MouseButtonPress)
//        {
//            gtInfo() << "TEST!";
//            gtMdiLauncher->open(GT_CLASSNAME(Nds3DPlot));
//        }
    }

    return false;
}

void
NdsShapeVisualizationModel::updateDrawing()
{
    int w = _label->width();
    int h = _label->height();

    _label->setPixmap(m_pixmap.scaled(w, h, Qt::KeepAspectRatio));
}

void
NdsShapeVisualizationModel::onInitialized()
{
    m_3dview->viewAxo();

    m_3dscene->setFaceBoundariesEnabled(true);
    m_3dscene->setFaceBoundariesLineWidth(0.5);

    QColor white(36, 49, 63);

    if (!gtApp->inDarkMode())
    {
        white = QColor(255, 255, 255);
    }

    m_3dview->setAxisCrossTextColor(1);
    m_3dview->setBackgroundGradient(white, white);
    m_3dview->setAxisCrossEnabled(false);
}

void
NdsShapeVisualizationModel::open3DPlot()
{
    GtMdiItem* mdiItem = gtMdiLauncher->open(GT_CLASSNAME(Nds3DPlot));

    Nds3DPlot* plot = qobject_cast<Nds3DPlot*>(mdiItem);

    if (plot) plot->addShapes(m_shapes);
}

unsigned int
NdsShapeVisualizationModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 0;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsShapeVisualizationModel::dataType(PortType const, PortIndex const) const
{
    return NdsShapeData().type();
}

std::shared_ptr<NodeData>
NdsShapeVisualizationModel::outData(PortIndex)
{
    return {};
}

void
NdsShapeVisualizationModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
//    gtInfo() << "NdsShapeVisualizationModel::setInData";

    m_shapes.clear();

    if (nodeData)
    {
        m_3dscene->clearAll();

        auto d = std::dynamic_pointer_cast<NdsShapeData>(nodeData);

        if (d)
        {
            m_shapes = d->shapes();

            foreach (ShapePtr shape, m_shapes)
            {
//                gtInfo() << "found shape! displaying...";

                QColor col = shape->getColor();
                shape->setMaterial(MAT_SILVER);
                shape->setColor(col);

                m_3dscene->updateShape(shape);
            }

            m_3dview->fitAll();

            QImage img;
            m_3dview->makeScreenshot(img, 400, 400);

            m_pixmap = QPixmap::fromImage(img);

            updateDrawing();
        }

    }
    else
    {
        m_3dscene->clearAll();
        m_pixmap = {};
        updateDrawing();
    }
}
