#ifndef GT_IGSHAPEGENERATORNODE_H
#define GT_IGSHAPEGENERATORNODE_H

#include "gt_intelligraphnode.h"
#include "models/data/gt_igobjectdata.h"

#include "gtl_shape.h"
#include "gtd_shapesettings.h"

class GtIgShapeGeneratorNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgShapeGeneratorNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData nodeData, PortIndex const port) override;

private:

    QList<ShapePtr> m_shapes;

    std::shared_ptr<GtIgObjectData> m_object;

    GtdShapeSettings m_settings;

    void generate();
};

#endif // GT_IGSHAPEGENERATORNODE_H
