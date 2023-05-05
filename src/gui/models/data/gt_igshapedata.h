#ifndef GT_IGSHAPEDATA_H
#define GT_IGSHAPEDATA_H

#include <QtNodes/NodeData>

#include "gtl_shape.h"

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class GtIgShapeData : public QtNodes::NodeData
{
public:
    GtIgShapeData() {}

    GtIgShapeData(QList<ShapePtr> shapes)
        : m_shapes(std::move(shapes))
    {}

    static QtNodes::NodeDataType const& staticType()
    {
        static const QtNodes::NodeDataType type = {
            QStringLiteral("shape"), QStringLiteral("Shape")
        };

        return type;
    }

    QtNodes::NodeDataType type() const override { return staticType(); }

    QList<ShapePtr> const& shapes() const { return m_shapes; }

private:
    QList<ShapePtr> m_shapes;
};

#endif // GT_IGSHAPEDATA_H
