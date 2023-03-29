#include "qwt_plot.h"

#include "nds_objectdata.h"

#include "nds_abstractqwtmodel.h"

NdsAbstractQwtModel::NdsAbstractQwtModel()
{

}

QWidget*
NdsAbstractQwtModel::embeddedWidget()
{
    return m_plot;
}

unsigned int
NdsAbstractQwtModel::nPorts(PortType portType) const
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
NdsAbstractQwtModel::dataType(PortType const, PortIndex const) const
{
    return NdsObjectData().type();
}

std::shared_ptr<NodeData>
NdsAbstractQwtModel::outData(PortIndex)
{
    return _nodeData;
}

void
NdsAbstractQwtModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
    _nodeData = nodeData;

    if (_nodeData)
    {
        auto d = std::dynamic_pointer_cast<NdsObjectData>(_nodeData);

        if (d->object())
        {

        }
    }
    else
    {

    }

    Q_EMIT dataUpdated(0);
}
