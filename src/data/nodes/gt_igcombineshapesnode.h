#ifndef GT_IGCOMBINESHAPESNODE_H
#define GT_IGCOMBINESHAPESNODE_H

#include "gt_intelligraphnode.h"

#include "gtl_shape.h"

class GtIgCombineShapesNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgCombineShapesNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData nodeData, PortIndex port) override;

protected slots:

    void inputConnectionCreated(ConnectionId const& id) override;

    void inputConnectionDeleted(ConnectionId const& id) override;

private:

    QList<QList<ShapePtr>> m_shapes;

    // Ports: 0, 1, 2, x
    QVector<PortIndex> m_connectedPorts{};

    QVector<PortIndex> m_unconnectedPorts{0, 1};

    void resizeShapes();
};

#endif // GT_IGCOMBINESHAPESNODE_H
