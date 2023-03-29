#ifndef NDSCOMBINESHAPESMODEL_H
#define NDSCOMBINESHAPESMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "gtl_shape.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class NdsCombineShapesModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsCombineShapesModel();

    ~NdsCombineShapesModel() = default;

    QString caption() const override { return QString("Combine Shapes"); }

    QString name() const override { return QString("Combine Shapes"); }

    virtual QString modelName() const { return QString("ShapeCombination"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, QtNodes::PortIndex portIndex) override;

    QWidget* embeddedWidget() override { return nullptr; }

    bool resizable() const override { return false; }

private:
    QList<ShapePtr> m_shapes_first;
    QList<ShapePtr> m_shapes_second;

};

#endif // NDSCOMBINESHAPESMODEL_H
