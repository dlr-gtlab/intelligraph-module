#ifndef NDSABSTRACTSHAPEMODEL_H
#define NDSABSTRACTSHAPEMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "gtl_shape.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class NdsAbstractShapeModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsAbstractShapeModel();

    ~NdsAbstractShapeModel() = default;

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    virtual QWidget* embeddedWidget() override { return nullptr; }

    virtual bool resizable() const override { return false; }

protected:
    virtual void compute(const QList<ShapePtr>& shapesIn,
                         QList<ShapePtr>& shapesOut) = 0;

private:
    QList<ShapePtr> m_shapes;

};

#endif // NDSABSTRACTSHAPEMODEL_H
