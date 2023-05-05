#ifndef GT_IGABSTRACTSHAPENODE_H
#define GT_IGABSTRACTSHAPENODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gtl_shape.h"

class GtIgAbstractShapeNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    GtIgAbstractShapeNode(QString const& caption, GtObject* parent = nullptr);

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData nodeData, PortIndex const port) override;

protected:

    virtual void compute(QList<ShapePtr> const& shapesIn,
                         QList<ShapePtr>& shapesOut) = 0;

private:

    QList<ShapePtr> m_shapes;
};

#endif // GT_IGABSTRACTSHAPENODE_H
