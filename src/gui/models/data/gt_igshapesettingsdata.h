#ifndef GT_IGSHAPESETTINGSDATA_H
#define GT_IGSHAPESETTINGSDATA_H

#include <QtNodes/NodeData>

#include "gtd_shapesettings.h"

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class GtIgShapeSettingsData : public QtNodes::NodeData
{
public:
    GtIgShapeSettingsData() {}

    GtIgShapeSettingsData(GtdShapeSettings settings)
        : m_settings(settings)
    {}

    static QtNodes::NodeDataType const& staticType()
    {
        static const QtNodes::NodeDataType type = {
            QStringLiteral("shapeSettings"), QStringLiteral("ShapeSettings")
        };

        return type;
    }

    QtNodes::NodeDataType type() const override { return staticType(); }

    GtdShapeSettings settings() const { return m_settings; }

private:

    GtdShapeSettings m_settings;
};

#endif // GT_IGSHAPESETTINGSDATA_H
