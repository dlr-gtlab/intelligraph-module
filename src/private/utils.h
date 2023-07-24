/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGUTILS_H
#define GT_IGUTILS_H

#include "gt_logstream.h"
#include "gt_igdoubledata.h"

#include <QtNodes/Definitions>

inline gt::log::Stream&
operator<<(gt::log::Stream& s, QtNodes::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeConnection["
            << con.outNodeId << ":" << con.outPortIndex << "/"
            << con.inNodeId  << ":" << con.inPortIndex  << "]";
    }
    return s;
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, std::shared_ptr<const GtIgNodeData> const& data)
{
    // temporary
    if (auto* d = qobject_cast<GtIgDoubleData const*>(data.get()))
    {
        gt::log::StreamStateSaver saver(s);
        return s.nospace() << data->metaObject()->className() << " (" <<d->value() << ")";
    }

    return s << (data ? data->metaObject()->className() : "nullptr");
}

#endif // GT_IGUTILS_H
