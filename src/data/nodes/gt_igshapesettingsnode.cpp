#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "models/data/gt_igshapesettingsdata.h"

#include "gt_igshapesettingsnode.h"

GTIG_REGISTER_NODE(GtIGShapeSettingsNode);

GtIGShapeSettingsNode::GtIGShapeSettingsNode() :
    GtIntelliGraphNode(("Shape Settings"))
{

}

unsigned int
GtIGShapeSettingsNode::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 0;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

GtIGShapeSettingsNode::NodeDataType
GtIGShapeSettingsNode::dataType(PortType const, PortIndex const) const
{
    return GtIgShapeSettingsData::staticType();
}

GtIGShapeSettingsNode::NodeData
GtIGShapeSettingsNode::outData(PortIndex)
{
    double rot = m_rot->value();

    if (m_reverseRotation->isChecked()) rot = -rot;

    m_settings.set(QStringLiteral("ROT_ANGLE"), rot);
    m_settings.set(QStringLiteral("CHILD_COMPONENTS"), m_childs->isChecked());
    m_settings.set(QStringLiteral("SINGLE_BLADE"), m_singleBlade->isChecked());
    m_settings.set(QStringLiteral("SOLID"), m_solidBlade->isChecked());

    return std::make_shared<GtIgShapeSettingsData>(m_settings);
}

QWidget*
GtIGShapeSettingsNode::embeddedWidget()
{
    if (!m_mainWidget) initWidget();
    return m_mainWidget;
}

void
GtIGShapeSettingsNode::initWidget()
{
    m_mainWidget = gt::ig::make_volatile<QWidget>();
    m_rot = new QDoubleSpinBox();
    m_childs = new QCheckBox();
    m_singleBlade = new QCheckBox();
    m_solidBlade = new QCheckBox();
    m_reverseRotation = new QCheckBox();

    auto* lay = new QFormLayout;

    m_rot->setMinimum(1.);
    m_rot->setMaximum(360.);
    m_rot->setValue(225.0);

    lay->addRow(tr("Rotation Angle"), m_rot);

    m_reverseRotation->setChecked(false);
    lay->addRow(tr("Reverse Rotation"), m_reverseRotation);

    m_childs->setChecked(false);
    lay->addRow(tr("Child Components"), m_childs);

    m_singleBlade->setChecked(false);
    lay->addRow(tr("Single Blade"), m_singleBlade);

    m_solidBlade->setChecked(false);
    lay->addRow(tr("Solid Blade"), m_solidBlade);

    m_mainWidget->setLayout(lay);

    connect(m_rot, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &GtIGShapeSettingsNode::settingsChanged);
    connect(m_reverseRotation, &QCheckBox::stateChanged,
            this, &GtIGShapeSettingsNode::settingsChanged);
    connect(m_childs, &QCheckBox::stateChanged,
            this, &GtIGShapeSettingsNode::settingsChanged);
    connect(m_singleBlade, &QCheckBox::stateChanged,
            this, &GtIGShapeSettingsNode::settingsChanged);
    connect(m_solidBlade, &QCheckBox::stateChanged,
            this, &GtIGShapeSettingsNode::settingsChanged);
}

void
GtIGShapeSettingsNode::settingsChanged()
{
    emit dataUpdated(0);
}
