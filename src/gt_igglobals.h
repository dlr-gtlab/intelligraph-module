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
#include "gt_typetraits.h"

#include <QPointF>

namespace gt
{
namespace ig
{

using NodeId = unsigned;

using PortIndex = unsigned;

using PortId = unsigned;

using Position = QPointF;

enum PortType
{
    In = 0,
    Out,
    NoType
};

template<typename T>
constexpr inline T invalid() noexcept { return std::numeric_limits<T>::max(); }

inline unsigned fromInt(GtIntProperty const& p)
{
    return p.get() >= 0 ? static_cast<unsigned>(p) : invalid<NodeId>();
}

} // namespace ig

namespace re
{

namespace ig
{

inline QRegExp forClassNames()
{
    return QRegExp(R"(^([a-zA-Z_][a-zA-Z0-9_]*::)*[a-zA-Z_][a-zA-Z0-9_]*$)");
}

} // namespace ig

} // namespace re

} // namespace gt

#endif // GT_IGGLOBALS_H
