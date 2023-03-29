#ifndef NDSEXAMPLEMODEL_H
#define NDSEXAMPLEMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class NdsExampleModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsExampleModel();

    ~NdsExampleModel() = default;

    QString caption() const override { return QString("IntelliGraph Node"); }

    QString name() const override { return QString("IntelliGraph Node"); }

    virtual QString modelName() const { return QString("IntelliGraph Node"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override { return nullptr; }

    bool resizable() const override { return false; }

};

#endif // NDSEXAMPLEMODEL_H
