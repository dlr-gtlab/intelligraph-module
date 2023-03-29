#ifndef NDSABSTRACTQWTMODEL_H
#define NDSABSTRACTQWTMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class QwtPlot;

class NdsAbstractQwtModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsAbstractQwtModel();

    ~NdsAbstractQwtModel() = default;

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    virtual QWidget* embeddedWidget() override;

    virtual bool resizable() const override { return true; }

protected:
    QwtPlot* m_plot;

private:
    std::shared_ptr<NodeData> _nodeData;

};

#endif // NDSABSTRACTQWTMODEL_H
