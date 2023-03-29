#ifndef NDSSHAPEGENMODEL_H
#define NDSSHAPEGENMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "gtl_shape.h"
#include "gtd_shapesettings.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class GtObject;

class NdsShapeGenModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsShapeGenModel();

    ~NdsShapeGenModel() = default;

    QString caption() const override { return QString("Shape Generator"); }

    QString name() const override { return QString("Shape Generator"); }

    virtual QString modelName() const { return QString("Shape"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override { return nullptr; }

    bool resizable() const override { return false; }

private:
    QList<ShapePtr> m_shapes;

    GtObject* m_obj;

    GtdShapeSettings m_settings;

    void generate();

};

#endif // NDSSHAPEGENMODEL_H
