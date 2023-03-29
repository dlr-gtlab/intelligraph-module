#ifndef NDSHELLOQMLMODEL_H
#define NDSHELLOQMLMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class QQuickWidget;

class NdsHelloQmlModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsHelloQmlModel();

    ~NdsHelloQmlModel() = default;

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    virtual QWidget* embeddedWidget() override;

    virtual bool resizable() const override { return true; }

protected:
     QQuickWidget* m_qmlWid;

};

#endif // NDSHELLOQMLMODEL_H
