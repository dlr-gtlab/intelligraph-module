#include "nds_examplemodel.h"

#include "models/data/gt_igobjectdata.h"
#include "models/data/gt_igshapedata.h"
#include "models/data/gt_igshapesettingsdata.h"

#include <stdexcept>

NdsExampleModel::NdsExampleModel()
{

}

unsigned int
NdsExampleModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = m_inDt.size();
        break;

    case PortType::Out:
        result = m_outDt.size();

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
        return m_inDt[portIndex];
        break;
    case PortType::Out:
       return m_outDt[portIndex];
    case PortType::None:
        break;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

std::shared_ptr<NodeData>
NdsExampleModel::outData(PortIndex)
{
    return {};
}

void
NdsExampleModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex portIndex)
{

}

void
NdsExampleModel::setInPortData(std::vector<QtNodes::NodeDataType> inDt)
{
    if (inDt.empty()) return;

    emit portsAboutToBeInserted(PortType::In, 0, inDt.size() - 1);
    m_inDt = inDt;
    emit portsInserted();
}

void
NdsExampleModel::setOutPortData(std::vector<QtNodes::NodeDataType> outDt)
{
    if (outDt.empty()) return;

    emit portsAboutToBeInserted(PortType::Out, 0, outDt.size() - 1);
    m_outDt = outDt;
    emit portsInserted();
}
