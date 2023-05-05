#ifndef GT_IGOBJECTDATA_H
#define GT_IGOBJECTDATA_H

#include <QtNodes/NodeData>
#include <QPointer>

#include "gt_object.h"

/// data for object pointers
class GtIgObjectData : public QtNodes::NodeData
{
public:

    explicit GtIgObjectData(GtObject const* obj = nullptr)
        : m_obj(std::unique_ptr<GtObject>(obj ? obj->clone() : nullptr))
    { }

    static QtNodes::NodeDataType const& staticType()
    {
        static const QtNodes::NodeDataType type = {
            QStringLiteral("object"), QStringLiteral("Object")
        };

        return type;
    }

    QtNodes::NodeDataType type() const override { return staticType(); }

    GtObject* object() const { return m_obj.get(); }

private:

    std::unique_ptr<GtObject> m_obj;
};

#endif // GT_IGOBJECTDATA_H
