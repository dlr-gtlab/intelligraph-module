#include "qwt_plot.h"

#include "models/data/gt_igobjectdata.h"

#include "gt_igabstractqwtnode.h"

GtIgAbstractQwtNode::GtIgAbstractQwtNode(QString const& caption, GtObject* parent) :
    GtIntelliGraphNode(caption, parent)
{
    setNodeFlag(gt::ig::Resizable);
}

unsigned int
GtIgAbstractQwtNode::nPorts(PortType portType) const
{
    switch (portType)
    {
    case PortType::In:
    case PortType::Out:
        return 1;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgAbstractQwtNode::NodeDataType
GtIgAbstractQwtNode::dataType(PortType const, PortIndex const) const
{
    return GtIgObjectData::staticType();
}

GtIgAbstractQwtNode::NodeData
GtIgAbstractQwtNode::outData(PortIndex)
{
    return _nodeData;
}

void
GtIgAbstractQwtNode::setInData(NodeData nodeData, PortIndex const)
{
    _nodeData = nodeData;
    emit dataUpdated(0);
}
