#include <QUuid>

#include "models/data/gt_igobjectdata.h"
#include "models/data/gt_igshapedata.h"
#include "models/data/gt_igshapesettingsdata.h"

#include "gtd_shapecreator.h"
#include "gtd_component.h"

#include "gt_igshapegeneratornode.h"

GTIG_REGISTER_NODE(GtIgShapeGeneratorNode);

GtIgShapeGeneratorNode::GtIgShapeGeneratorNode() :
    GtIntelliGraphNode(tr("Shape Generator"))
{

}

unsigned int
GtIgShapeGeneratorNode::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        return 2;
    case PortType::Out:
        return 1;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgShapeGeneratorNode::NodeDataType
GtIgShapeGeneratorNode::dataType(PortType const type, PortIndex const idx) const
{
    switch (type) {
    case PortType::In:
        switch (idx)
        {
        case 0:
            return GtIgShapeSettingsData::staticType();
        case 1:
            return GtIgObjectData::staticType();
        }
        break;
    case PortType::Out:
       return GtIgShapeData::staticType();
    case PortType::None:
        break;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgShapeGeneratorNode::NodeData
GtIgShapeGeneratorNode::outData(PortIndex)
{
    return std::make_shared<GtIgShapeData>(m_shapes);
}

void
GtIgShapeGeneratorNode::setInData(NodeData nodeData, PortIndex port)
{
    m_shapes.clear();

    switch (port)
    {
    case 0:
    {
        if (auto settingsData = gt::ig::nodedata_cast<GtIgShapeSettingsData>(
                std::move(nodeData)))
        {
            m_settings = settingsData->settings();
            return generate();
        }
        m_settings = GtdShapeSettings();
        break;
    }
    case 1:
    {
        if (auto objectData = gt::ig::nodedata_cast<GtIgObjectData>(
                std::move(nodeData)))
        {
            m_object = std::move(objectData);
            return generate();
        }
        m_object.reset();
        break;
    }
    default:
        break;
    }

    emit dataInvalidated(0);
}

void
GtIgShapeGeneratorNode::generate()
{
    if (!m_object)
    {
        emit dataInvalidated(0);
        return;
    }

    if (auto* comp = qobject_cast<GtdComponent*>(m_object->object()))
    {
        GtdShapeCreator creator;
        m_shapes = creator.create3DShape(comp, m_settings);

        for (ShapePtr const& shape : m_shapes)
        {
            shape->setUuid(QUuid::createUuid().toString());
        }

        return emit dataUpdated(0);
    }

    emit dataInvalidated(0);
}
