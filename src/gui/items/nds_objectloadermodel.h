#ifndef NDSOBJECTLOADERMODEL_H
#define NDSOBJECTLOADERMODEL_H

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class GtObject;

class NdsObjectLoaderModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsObjectLoaderModel();

    ~NdsObjectLoaderModel() = default;

    QString caption() const override { return QString("Object Source"); }

    QString name() const override { return QString("Object Source"); }

    virtual QString modelName() const { return QString("Source Object"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override {}

    QWidget *embeddedWidget() override { return _label; }

//    bool resizable() const override { return true; }

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QLabel* _label;

    GtObject* m_obj;

};

#endif // NDSOBJECTLOADERMODEL_H
