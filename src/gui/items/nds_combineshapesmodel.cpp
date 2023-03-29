#include "nds_shapedata.h"

#include "nds_combineshapesmodel.h"

NdsCombineShapesModel::NdsCombineShapesModel()
{

}

unsigned int
NdsCombineShapesModel::nPorts(PortType portType) const
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
NdsCombineShapesModel::dataType(PortType const, PortIndex const) const
{
    return NdsShapeData().type();
}

std::shared_ptr<NodeData>
NdsCombineShapesModel::outData(PortIndex)
{
    QList<ShapePtr> combined;
    combined << m_shapes_first << m_shapes_second;

    return std::make_shared<NdsShapeData>(combined);
}

void
NdsCombineShapesModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{

    auto objData = std::dynamic_pointer_cast<NdsShapeData>(nodeData);

    if (portIndex == 0)
    {
        m_shapes_first.clear();
    }
    else if (portIndex == 1)
    {
        m_shapes_second.clear();
    }

    if (objData)
    {
        if (portIndex == 0)
        {
            m_shapes_first << objData->shapes();
        }
        else if (portIndex == 1)
        {
            m_shapes_second << objData->shapes();
        }
    }


    Q_EMIT dataUpdated(0);
}
