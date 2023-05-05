#ifndef GT_IGSHAPEVISUALIZATIONNODE_H
#define GT_IGSHAPEVISUALIZATIONNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gtl_shape.h"

#include <QPixmap>

class Scene3d;
class View3d;
class QLabel;
class NdsLabel;

class GtIgShapeVisualizationNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgShapeVisualizationNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    void setInData(NodeData nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override;

    void open3DPlot();

protected:

    bool eventFilter(QObject *object, QEvent *event) override;

private:

    gt::ig::volatile_ptr<QWidget> m_canvas;

    NdsLabel* m_label{};
    /// 3D view widget
    View3d* m_view{};
    /// 3D scene widget
    Scene3d* m_scene{};

    QPixmap m_pixmap;

    QList<ShapePtr> m_shapes;

    void updateDrawing();

    void initWidget();

private slots:

    void onInitialized();
};

#endif // GT_IGSHAPEVISUALIZATIONNODE_H
