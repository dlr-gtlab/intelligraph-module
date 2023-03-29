#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "nds_shapesettingsdata.h"

#include "nds_shapesettingsmodel.h"

NdsShapeSettingsModel::NdsShapeSettingsModel() :
    m_mainWid(new QWidget()),
    m_rot(new QDoubleSpinBox()),
    m_childs(new QCheckBox()),
    m_singleBlade(new QCheckBox()),
    m_solidBlade(new QCheckBox()),
    m_reverseRotation(new QCheckBox())
{
   auto* lay = new QFormLayout;

   m_rot->setMinimum(1.);
   m_rot->setMaximum(360.);
   m_rot->setValue(225.0);

   lay->addRow("Rotation Angle", m_rot);

   m_reverseRotation->setChecked(false);
   lay->addRow("Reverse Rotation", m_reverseRotation);

   m_childs->setChecked(false);
   lay->addRow("Child Components", m_childs);

   m_singleBlade->setChecked(false);
   lay->addRow("Single Blade", m_singleBlade);

   m_solidBlade->setChecked(false);
   lay->addRow("Solid Blade", m_solidBlade);

   m_mainWid->setLayout(lay);

   connect(m_rot, SIGNAL(valueChanged(double)), SLOT(settingsChanged()));
   connect(m_reverseRotation, SIGNAL(stateChanged(int)), SLOT(settingsChanged()));
   connect(m_childs, SIGNAL(stateChanged(int)), SLOT(settingsChanged()));
   connect(m_singleBlade, SIGNAL(stateChanged(int)), SLOT(settingsChanged()));
   connect(m_solidBlade, SIGNAL(stateChanged(int)), SLOT(settingsChanged()));
}

unsigned int
NdsShapeSettingsModel::nPorts(PortType portType) const
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

NodeDataType
NdsShapeSettingsModel::dataType(PortType const, PortIndex const) const
{
    return NdsShapeSettingsData().type();
}

std::shared_ptr<NodeData>
NdsShapeSettingsModel::outData(PortIndex)
{
    double rot = m_rot->value();

    if (m_reverseRotation->isChecked())
    {
        rot = -rot;
    }

    m_settings.set("ROT_ANGLE", rot);
    m_settings.set("CHILD_COMPONENTS", m_childs->isChecked());
    m_settings.set("SINGLE_BLADE", m_singleBlade->isChecked());
    m_settings.set("SOLID", m_solidBlade->isChecked());

    return std::make_shared<NdsShapeSettingsData>(m_settings);
}

QWidget*
NdsShapeSettingsModel::embeddedWidget()
{
    return m_mainWid;
}

void
NdsShapeSettingsModel::settingsChanged()
{
    Q_EMIT dataUpdated(0);
}
