#ifndef NDS_SHAPEDATA_H
#define NDS_SHAPEDATA_H

#include <QtNodes/NodeData>

#include "gtl_shape.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NdsShapeData : public NodeData
{
public:
    NdsShapeData() {}

    NdsShapeData(QList<ShapePtr> shapes)
        : m_shapes(shapes)
    {}

    NodeDataType type() const override
    {
        //       id      name
        return {"shape", "Shape"};
    }

    QList<ShapePtr> shapes() const { return m_shapes; }

private:
    QList<ShapePtr> m_shapes;
};

#endif // NDS_SHAPEDATA_H
