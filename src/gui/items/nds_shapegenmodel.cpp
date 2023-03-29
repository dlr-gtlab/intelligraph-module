#include <QUuid>

#include "nds_objectdata.h"
#include "nds_shapedata.h"
#include "nds_shapesettingsdata.h"

#include "gtd_shapecreator.h"
#include "gtd_component.h"

#include "nds_shapegenmodel.h"

NdsShapeGenModel::NdsShapeGenModel() :
    m_obj(nullptr)
{

}

unsigned int
NdsShapeGenModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 2;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsShapeGenModel::dataType(PortType const portType, PortIndex const portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0:
            return NdsShapeSettingsData().type();
        case 1:
            return NdsObjectData().type();
        }
        break;

    case PortType::Out:
       return NdsShapeData().type();

    case PortType::None:
        break;
    }
    // FIXME: control may reach end of non-void function [-Wreturn-type]
    return NodeDataType();
}

std::shared_ptr<NodeData>
NdsShapeGenModel::outData(PortIndex)
{
    return std::make_shared<NdsShapeData>(m_shapes);
}

void
NdsShapeGenModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{
    m_shapes.clear();

    if (portIndex == 0)
    {
        auto objData = std::dynamic_pointer_cast<NdsShapeSettingsData>(nodeData);

        if (!objData) {
            m_settings = GtdShapeSettings();

            Q_EMIT dataInvalidated(0);
            Q_EMIT dataUpdated(0);
        }
        else
        {
            m_settings = objData->settings();
        }


    }
    else if (portIndex == 1)
    {
        auto objData = std::dynamic_pointer_cast<NdsObjectData>(nodeData);

        if (!objData) {
            m_obj = nullptr;

            Q_EMIT dataInvalidated(0);
            Q_EMIT dataUpdated(0);
        }
        else
        {
            m_obj = objData->object();
        }

    }

    generate();
}

void
NdsShapeGenModel::generate()
{
    if (!m_obj)
    {
        Q_EMIT dataInvalidated(0);
        return;
    }

    GtdComponent* comp = qobject_cast<GtdComponent*>(m_obj);

    if (!comp)
    {
        Q_EMIT dataInvalidated(0);
        return;
    }

    GtdShapeCreator creator;
    m_shapes = creator.create3DShape(comp, m_settings);

    foreach (ShapePtr shape, m_shapes)
    {
        shape->setUuid(QUuid::createUuid().toString());
    }

    Q_EMIT dataUpdated(0);
}
