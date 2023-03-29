#ifndef NDSSHAPESETTINGSMODEL_H
#define NDSSHAPESETTINGSMODEL_H

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "gtd_shapesettings.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class QDoubleSpinBox;
class QCheckBox;

class NdsShapeSettingsModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NdsShapeSettingsModel();

    ~NdsShapeSettingsModel() = default;

    QString caption() const override { return QString("Shape Settings"); }

    QString name() const override { return QString("Shape Settings"); }

    virtual QString modelName() const { return QString("Shape Settings"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override {}

    QWidget* embeddedWidget() override;

private:
    GtdShapeSettings m_settings;

    QWidget* m_mainWid;

    QDoubleSpinBox* m_rot;

    QCheckBox* m_childs;

    QCheckBox* m_singleBlade;

    QCheckBox* m_solidBlade;

    QCheckBox* m_reverseRotation;

private slots:
    void settingsChanged();

};

#endif // NDSSHAPESETTINGSMODEL_H
