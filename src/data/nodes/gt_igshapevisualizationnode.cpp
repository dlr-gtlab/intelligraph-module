#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QEvent>
#include <QBasicTimer>

#include "nds_3dplot.h"

#include "gt_mdilauncher.h"
#include "gt_mdiitem.h"
#include "gt_application.h"

#include "gtl_scene3d.h"
#include "gtl_view3d.h"

#include "gt_igshapevisualizationnode.h"
#include "models/data/gt_igshapedata.h"

GTIG_REGISTER_NODE(GtIgShapeVisualizationNode);

class NdsLabel : public QLabel
{
public:
    NdsLabel(const QString& str, GtIgShapeVisualizationNode* model) :
        QLabel(str), m_model(model)
    {

    }

    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        m_model->open3DPlot();
    }

    GtIgShapeVisualizationNode* m_model;
};

GtIgShapeVisualizationNode::GtIgShapeVisualizationNode() :
    GtIntelliGraphNode(tr("Shape Viewer"))
{
    setNodeFlag(gt::ig::Resizable);
}

QWidget*
GtIgShapeVisualizationNode::embeddedWidget()
{
    if (!m_canvas) initWidget();
    return m_canvas;
}

bool
GtIgShapeVisualizationNode::eventFilter(QObject *object, QEvent *event)
{
    if (!m_pixmap.isNull() && object == m_label)
    {
        int w = m_label->width();
        int h = m_label->height();

        if (event->type() == QEvent::Resize)
        {
            m_label->setPixmap(m_pixmap.scaled(w, h, Qt::KeepAspectRatio));
        }
    }

    return false;
}

void
GtIgShapeVisualizationNode::updateDrawing()
{
    if (m_pixmap.isNull())
    {
        return m_label->setPixmap({});
    }

    int w = m_label->width();
    int h = m_label->height();

    m_label->setPixmap(m_pixmap.scaled(w, h, Qt::KeepAspectRatio));
}

void
GtIgShapeVisualizationNode::initWidget()
{
    m_canvas = gt::ig::make_volatile<QWidget>();

    m_label = new NdsLabel(tr("Open Shape Viewer"), this);

    m_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_label->setMinimumSize(200, 200);

    if (gtApp->inDarkMode())
    {
        m_label->setStyleSheet("QLabel { background-color : rgb(36, 49, 63); color : blue; }");
    }
    else
    {
        m_label->setStyleSheet("QLabel { background-color : rgb(255, 255, 255); color : blue; }");
    }
    m_label->installEventFilter(this);

    m_view = new View3d(m_canvas);

    QVBoxLayout* lay = new QVBoxLayout;
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    m_canvas->setLayout(lay);

    lay->addWidget(m_label);

    m_scene = new Scene3d();

    connect(m_view, SIGNAL(initialized()), this, SLOT(onInitialized()));

    m_view->setScene(m_scene);
}

void
GtIgShapeVisualizationNode::onInitialized()
{
    m_view->viewAxo();

    m_scene->setFaceBoundariesEnabled(true);
    m_scene->setFaceBoundariesLineWidth(0.5);

    QColor white(36, 49, 63);

    if (!gtApp->inDarkMode())
    {
        white = QColor(255, 255, 255);
    }

    m_view->setAxisCrossTextColor(1);
    m_view->setBackgroundGradient(white, white);
    m_view->setAxisCrossEnabled(false);
}

void
GtIgShapeVisualizationNode::open3DPlot()
{
    GtMdiItem* mdiItem = gtMdiLauncher->open(GT_CLASSNAME(Nds3DPlot));

    if (Nds3DPlot* plot = qobject_cast<Nds3DPlot*>(mdiItem))
    {
        plot->addShapes(m_shapes);
    }
}

unsigned int
GtIgShapeVisualizationNode::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        return 1;
    case PortType::Out:
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgShapeVisualizationNode::NodeDataType
GtIgShapeVisualizationNode::dataType(PortType const, PortIndex const) const
{
    return GtIgShapeData::staticType();
}

void
GtIgShapeVisualizationNode::setInData(NodeData nodeData, PortIndex const port)
{
    m_shapes.clear();
    m_scene->clearAll();
    m_pixmap = {};

    if (auto shapeData = gt::ig::nodedata_cast<GtIgShapeData>(std::move(nodeData)))
    {
        m_shapes = shapeData->shapes();

        for (ShapePtr const& shape : qAsConst(m_shapes))
        {
            QColor col = shape->getColor();
            shape->setMaterial(MAT_SILVER);
            shape->setColor(col);

            m_scene->updateShape(shape);
        }

        m_view->fitAll();

        QImage img;
        m_view->makeScreenshot(img, 400, 400);

        m_pixmap = QPixmap::fromImage(img);
    }

    updateDrawing();
}
