#include "models/data/gt_igshapedata.h"

#include "gt_igcombineshapesnode.h"

GTIG_REGISTER_NODE(GtIgCombineShapesNode);

GtIgCombineShapesNode::GtIgCombineShapesNode() :
    GtIntelliGraphNode(tr("Combine Shapes"))
{
    resizeShapes();
}

unsigned int
GtIgCombineShapesNode::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
        return m_connectedPorts.size() + m_unconnectedPorts.size();
    case PortType::Out:
        return 1;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};
}

GtIgCombineShapesNode::NodeDataType
GtIgCombineShapesNode::dataType(PortType const, PortIndex const) const
{
    return GtIgShapeData::staticType();
}

GtIgCombineShapesNode::NodeData
GtIgCombineShapesNode::outData(PortIndex)
{
    QList<ShapePtr> shapes;
    shapes.reserve(std::accumulate(std::cbegin(m_shapes),
                                   std::cend(m_shapes),
                                   int{0}, [](int sum, QList<ShapePtr> const& list){
        return sum + list.size();
    }));

    std::for_each(std::cbegin(m_shapes), std::cend(m_shapes), [&](QList<ShapePtr> const& list){
        std::copy(std::cbegin(list), std::cend(list), std::back_inserter(shapes));
    });

    return std::make_shared<GtIgShapeData>(std::move(shapes));
}

void
GtIgCombineShapesNode::setInData(NodeData nodeData, PortIndex port)
{
    auto objData = gt::ig::nodedata_cast<GtIgShapeData>(std::move(nodeData));

    // port should now be within the bounds
    if (port < m_shapes.size())
    {
        m_shapes[port] = objData ? objData->shapes() : QList<ShapePtr>();

        emit dataUpdated(0);
    }
}

void
GtIgCombineShapesNode::resizeShapes()
{
    auto target = m_connectedPorts.size() + m_unconnectedPorts.size();

    // fill up shapes if there are too few entries
    for (int i = 0; i < target; ++i)
    {
        m_shapes.append(QList<ShapePtr>{});
    }
    // remove entries if there are too many
    for (int i = m_shapes.size(); i > target; --i)
    {
        m_shapes.removeLast();
    }
}

void
GtIgCombineShapesNode::inputConnectionCreated(const ConnectionId& id)
{
    assert(!m_unconnectedPorts.empty());

    auto portIdx = id.inPortIndex;

    // sanity checks
    if (!m_unconnectedPorts.contains(portIdx))
    {
        gtError() << "inserting port:"
                  << tr("Unconnected ports does not contain port index %1!")
                     .arg(portIdx);
        gtError() << m_connectedPorts << "vs" << m_unconnectedPorts;
        return;
    }
    if (m_connectedPorts.contains(portIdx))
    {
        gtError() << "inserting port:"
                  << tr("Port %1 is already connected!")
                     .arg(portIdx);
        gtError() << m_connectedPorts << "vs" << m_unconnectedPorts;
        return;
    }

    auto lastPortIdx = m_unconnectedPorts.last();

    // move port to connected ports
    m_connectedPorts.append(portIdx);
    m_unconnectedPorts.removeOne(portIdx);

    std::sort(std::begin(m_connectedPorts), std::end(m_connectedPorts));

    // we have to make sure there is always one port available
    bool appendPort = m_unconnectedPorts.empty() ||
                      *std::max_element(std::cbegin(m_connectedPorts), std::cend(m_connectedPorts)) >
                      *std::max_element(std::cbegin(m_unconnectedPorts), std::cend(m_unconnectedPorts));
    if (appendPort)
    {
        emit portsAboutToBeInserted(PortType::In, lastPortIdx, lastPortIdx);

        // append port
        m_unconnectedPorts.append(lastPortIdx + 1);
        std::sort(std::begin(m_unconnectedPorts), std::end(m_unconnectedPorts));

        resizeShapes();

        emit portsInserted();
    }
}

void
GtIgCombineShapesNode::inputConnectionDeleted(const ConnectionId& id)
{
    auto portIdx = id.inPortIndex;

    // sanity checks
    if (!m_connectedPorts.contains(portIdx))
    {
        gtError() << "deleting port:"
                  << tr("Port %1 is not connected!")
                     .arg(portIdx);
        gtError() << m_connectedPorts << "vs" << m_unconnectedPorts;
        return;
    }
    if (m_unconnectedPorts.contains(portIdx))
    {
        gtError() << "deleting port:"
                  << tr("Port %1 is already disconnected!")
                     .arg(portIdx);
        gtError() << m_connectedPorts << "vs" << m_unconnectedPorts;
        return;
    }

    // clear shapes at entry
    assert(m_shapes.size() > portIdx);
    m_shapes[portIdx].clear();

    // move port to unconnected ports
    m_connectedPorts.removeOne(portIdx);
    m_unconnectedPorts.append(portIdx);

    std::sort(std::begin(m_unconnectedPorts), std::end(m_unconnectedPorts));

    // e.g. say ports (0, 2) are connected and (1, 3, 4) are now disconnected
    // - we want to remove the excess ports (i.e. 4)
    // - 1. we find the last connected port (i.e. 2)
    // - 2. we keep the next port (i.e. 3)
    // - 3. we will remove all other ports (here >= 4)

    PortIndex lastConnectedPortIdx = m_connectedPorts.empty() ? 0 :
                    *std::max_element(std::cbegin(m_connectedPorts),
                                      std::cend(m_connectedPorts));

    PortIndex excessPortStart = m_unconnectedPorts.indexOf(lastConnectedPortIdx + 1);
    PortIndex excessPortEnd   = m_unconnectedPorts.last();
    if (excessPortStart < 0)
    {
        gtWarning() << "excess port not found!";
        return;
    }

    emit portsAboutToBeDeleted(PortType::In, excessPortStart, excessPortEnd);

    m_unconnectedPorts = m_unconnectedPorts.mid(0, excessPortStart + 1);

    emit portsDeleted();
}
