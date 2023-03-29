#include "nds_shapedata.h"

#include "nds_abstractshapemodel.h"

NdsAbstractShapeModel::NdsAbstractShapeModel()
{

}

unsigned int
NdsAbstractShapeModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsAbstractShapeModel::dataType(PortType const, PortIndex const) const
{
    return NdsShapeData().type();
}

std::shared_ptr<NodeData>
NdsAbstractShapeModel::outData(PortIndex)
{
    return std::make_shared<NdsShapeData>(m_shapes);
}

void
NdsAbstractShapeModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{

    auto objData = std::dynamic_pointer_cast<NdsShapeData>(nodeData);

    m_shapes.clear();

    if (!objData) {
        Q_EMIT dataInvalidated(0);
    }
    else
    {
        compute(objData->shapes(), m_shapes);
    }

    Q_EMIT dataUpdated(0);
}
