#ifndef NDS_SHAPESETTINGSDATA_H
#define NDS_SHAPESETTINGSDATA_H

#include <QtNodes/NodeData>

#include "gtd_shapesettings.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NdsShapeSettingsData : public NodeData
{
public:
    NdsShapeSettingsData() {}

    NdsShapeSettingsData(GtdShapeSettings settings)
        : m_settings(settings)
    {}

    NodeDataType type() const override
    {
        //       id      name
        return {"shapeSettings", "ShapeSettings"};
    }

    GtdShapeSettings settings() const { return m_settings; }

private:
    GtdShapeSettings m_settings;
};

#endif // NDS_SHAPESETTINGSDATA_H
