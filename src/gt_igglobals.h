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

#include <QPointF>

namespace gt
{
namespace ig
{

// Base class for typesafe type aliases
template <typename T, typename Tag>
class StrongType
{
public:

    using value_type = T;

    constexpr inline explicit StrongType(T value = {}) noexcept :
        m_value(std::move(value))
    {}
    StrongType(StrongType const&) = default;
    StrongType(StrongType&&) = default;
    StrongType& operator=(StrongType const& value) = default;
    StrongType& operator=(StrongType&& value) = default;
    ~StrongType() = default;

    template<typename U>
    static StrongType fromValue(U value) { return StrongType{static_cast<T>(value)}; }

    // Overload comparison operators as needed
    constexpr inline bool
    operator==(StrongType const& other) const noexcept { return m_value == other.m_value; }

    constexpr inline bool
    operator!=(StrongType const& other) const noexcept { return !(*this == other); }

    constexpr inline StrongType&
    operator+=(StrongType const& o) noexcept { m_value + o.m_value; return *this; }
    constexpr inline StrongType&
    operator-=(StrongType const& o) noexcept { m_value - o.m_value; return *this; }
    constexpr inline StrongType&
    operator*=(StrongType const& o) noexcept { m_value * o.m_value; return *this; }
    constexpr inline StrongType&
    operator/=(StrongType const& o) noexcept { m_value / o.m_value; return *this; }

    // pre increment
    constexpr inline StrongType&
    operator++() noexcept { ++m_value; return *this; }
    // post increment
    constexpr inline StrongType
    operator++(int) noexcept { StrongType tmp(*this); operator++(); return tmp; }

    operator T() const { return value(); }

    // Access the underlying value
    constexpr inline T
    value() const { return m_value; }

private:
    T m_value = std::numeric_limits<T>::max();
};

using NodeId    = StrongType<unsigned, struct NodeId_>;
using PortIndex = StrongType<unsigned, struct PortIndex_>;
using PortId    = StrongType<unsigned, struct PortId_>;

using Position = QPointF;

enum PortType
{
    In = 0,
    Out,
    NoType
};

template<typename T>
constexpr inline T invalid() noexcept
{
    return T{ std::numeric_limits<typename T::value_type>::max() };
}

inline unsigned fromInt(GtIntProperty const& p) noexcept
{
    return p.get() >= 0 ? static_cast<unsigned>(p) :
                          std::numeric_limits<unsigned>::max();
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

template <typename T, typename Tag>
constexpr inline bool
operator+=(gt::ig::StrongType<T, Tag> const& a,
           gt::ig::StrongType<T, Tag> const& b) noexcept { return a += b; }

template <typename T, typename Tag>
constexpr inline bool
operator-=(gt::ig::StrongType<T, Tag> const& a,
           gt::ig::StrongType<T, Tag> const& b) noexcept { return a -= b; }

template <typename T, typename Tag>
constexpr inline bool
operator*=(gt::ig::StrongType<T, Tag> const& a,
           gt::ig::StrongType<T, Tag> const& b) noexcept{ return a *= b; }

template <typename T, typename Tag>
constexpr inline bool
operator/=(gt::ig::StrongType<T, Tag> const& a,
           gt::ig::StrongType<T, Tag> const& b) noexcept { return a /= b; }

template <typename T, typename Tag>
inline gt::log::Stream& operator<<(gt::log::Stream& s, gt::ig::StrongType<T, Tag> const& t)
{
    return s << t.value();
}

#endif // GT_IGGLOBALS_H
