#include "models/data/gt_igshapedata.h"

#include "gt_igabstractshapenode.h"

GtIgAbstractShapeNode::GtIgAbstractShapeNode(QString const& caption, GtObject* parent) :
    GtIntelliGraphNode(caption, parent)
{

}

unsigned int
GtIgAbstractShapeNode::nPorts(PortType portType) const
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

GtIgAbstractShapeNode::NodeDataType
GtIgAbstractShapeNode::dataType(PortType const, PortIndex const) const
{
    return GtIgShapeData::staticType();
}

GtIgAbstractShapeNode::NodeData
GtIgAbstractShapeNode::outData(PortIndex)
{
    return std::make_shared<GtIgShapeData>(m_shapes);
}

void
GtIgAbstractShapeNode::setInData(NodeData nodeData, PortIndex const)
{
    auto objData = gt::ig::nodedata_cast<GtIgShapeData>(std::move(nodeData));

    m_shapes.clear();

    if (!objData)
    {
        emit dataInvalidated(0);
        return;
    }

    compute(objData->shapes(), m_shapes);
    emit dataUpdated(0);
}
