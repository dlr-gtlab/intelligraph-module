/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGGLOBALS_H
#define GT_IGGLOBALS_H

#include "gt_intproperty.h"
#include "gt_logging.h"
#include "gt_qtutilities.h"
#include "gt_typetraits.h"

#include <QtNodes/Definitions>
#include <QtNodes/NodeData>

namespace gt
{
namespace ig
{

using NodeId = unsigned;

using PortIdx = unsigned;

using Position = QPointF;

inline unsigned fromInt(GtIntProperty const& p)
{
    return p.get() >= 0 ? static_cast<unsigned>(p) : std::numeric_limits<NodeId>::max();
}

inline auto makeUniqueName(QObject const& obj, QString const& name = {})
{
    auto objectName = name.isEmpty() ? obj.objectName() : name;

    if (auto* parent = obj.parent())
    {
        auto list = parent->findChildren<QObject const*>(
                        QString{}, Qt::FindDirectChildrenOnly);

        list.removeOne(&obj);

        return gt::detail::makeUniqueNameImpl(objectName, list,
                                              [](QObject const* o){
            return o->objectName();
        });
    }

    return objectName;
}

inline auto makeUniqueName(QString const& str, QObject const* obj)
{
    return obj ? gt::makeUniqueName(str, *obj) : str;
}

inline auto makeUniqueName(QString const& str, QObject const& obj)
{
    return gt::makeUniqueName(str, obj);
}

inline void setUniqueName(QObject& obj, QString const& name = {})
{
    obj.setObjectName(makeUniqueName(obj, name));
}

template <typename T, typename U,
          gt::trait::enable_if_base_of<QtNodes::NodeData, U> = true,
          gt::trait::enable_if_base_of<QtNodes::NodeData, T> = true>
inline std::shared_ptr<T> nodedata_cast(std::shared_ptr<U> ptr)
{
    if (ptr && ptr->type().id == T::staticType().id)
    {
        return std::static_pointer_cast<T>(ptr);
    }
    return {};
}

} // namespace ig

} // namespace gt

inline gt::log::Stream& operator<<(gt::log::Stream& s, QtNodes::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
          << "NodeConnection["
          << con.inNodeId  << ":" << con.inPortIndex << "/"
          << con.outNodeId << ":" << con.outPortIndex << "]";
    }
    return s;
}

#endif // GT_IGGLOBALS_H
