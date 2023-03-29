#ifndef NDSOBJECTMEMENTOMODEL_H
#define NDSOBJECTMEMENTOMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class GtCodeEditor;

class NdsObjectMementoModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsObjectMementoModel();

    ~NdsObjectMementoModel() = default;

    QString caption() const override { return QString(" "); }

    QString name() const override { return QString("Memento Viewer"); }

    virtual QString modelName() const { return QString("Object Memento"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override;

    bool resizable() const override { return true; }

private:
    std::shared_ptr<NodeData> _nodeData;

    GtCodeEditor* m_editor;
};

#endif // NDSOBJECTMEMENTOMODEL_H
