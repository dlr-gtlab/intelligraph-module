#ifndef NDSSHAPEVISUALIZATIONMODEL_H
#define NDSSHAPEVISUALIZATIONMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "gtl_shape.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class Scene3d;
class View3d;
class QLabel;
class NdsLabel;

class NdsShapeVisualizationModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsShapeVisualizationModel();

    ~NdsShapeVisualizationModel() = default;

    QString caption() const override { return QString("Shape Drawer"); }

    QString name() const override { return QString("Shape Drawer"); }

    virtual QString modelName() const { return QString("Shape Visualization"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override;

    bool resizable() const override { return true; }

    void open3DPlot();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    /// 3D scene widget
    Scene3d* m_3dscene;

    /// 3D view widget
    View3d* m_3dview;

    QWidget* m_canvas;

    NdsLabel* _label;

    QPixmap m_pixmap;

    QList<ShapePtr> m_shapes;

    void updateDrawing();

private slots:
    void onInitialized();

};

#endif // NDSSHAPEVISUALIZATIONMODEL_H
