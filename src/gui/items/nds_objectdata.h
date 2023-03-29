#ifndef NDS_OBJECTDATA_H
#define NDS_OBJECTDATA_H

#include <QtNodes/NodeData>

#include "gt_object.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NdsObjectData : public NodeData
{
public:
    NdsObjectData() {}

    NdsObjectData(GtObject* obj)
        : m_obj(obj)
    {}

    NodeDataType type() const override
    {
        //       id      name
        return {"object", "Object"};
    }

    GtObject* object() const { return m_obj; }

private:
    GtObject* m_obj;
};

#endif // NDS_OBJECTDATA_H
