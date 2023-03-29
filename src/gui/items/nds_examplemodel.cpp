#include "nds_objectdata.h"
#include "nds_shapedata.h"
#include "nds_shapesettingsdata.h"

#include "nds_examplemodel.h"

NdsExampleModel::NdsExampleModel()
{

}

unsigned int
NdsExampleModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 4;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsExampleModel::dataType(PortType const portType, PortIndex const portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0:
            return NdsShapeSettingsData().type();
        case 1:
            return NdsObjectData().type();
        case 2:
            return NdsShapeSettingsData().type();
        case 3:
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
NdsExampleModel::outData(PortIndex)
{
    return std::make_shared<NdsShapeData>();
}

void
NdsExampleModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{

}
