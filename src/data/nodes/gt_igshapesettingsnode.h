#ifndef GT_IGSHAPESETTINGSNODE_H
#define GT_IGSHAPESETTINGSNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gtd_shapesettings.h"

class QDoubleSpinBox;
class QCheckBox;
class GtIGShapeSettingsNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIGShapeSettingsNode();

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    NodeData outData(PortIndex const port) override;

    QWidget* embeddedWidget() override;

private:

    GtdShapeSettings m_settings;

    gt::ig::volatile_ptr<QWidget> m_mainWidget{};

    QDoubleSpinBox* m_rot{};

    QCheckBox* m_childs{};

    QCheckBox* m_singleBlade{};

    QCheckBox* m_solidBlade{};

    QCheckBox* m_reverseRotation{};

    void initWidget();

private slots:

    void settingsChanged();
};

#endif // GT_IGSHAPESETTINGSNODE_H
