#ifndef GT_IGSTRINGLISTDATA_H
#define GT_IGSTRINGLISTDATA_H

#include <QtNodes/NodeData>
#include <QPointer>

#include "gt_object.h"

/// data for string lists
class GtIgStringListData : public QtNodes::NodeData
{
public:

    explicit GtIgStringListData(QStringList list = {})
        : m_list(std::move(list))
    { }


    static QtNodes::NodeDataType const& staticType()
    {
        static const QtNodes::NodeDataType type = {
            QStringLiteral("stringlist"), QStringLiteral("StringList")
        };

        return type;
    }

    QtNodes::NodeDataType type() const override { return staticType(); }

    QStringList const& values() const { return m_list; }

private:

    QStringList m_list;
};

#endif // GT_IGSTRINGLISTDATA_H
