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

class GtIntelliGraphNode;
class GtIgNodeData;

namespace gt
{
namespace ig
{

// Base class for typesafe type aliases
template <typename T, typename Tag, T InitValue = 0>
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
    operator+=(StrongType const& o) noexcept { m_value += o.m_value; return *this; }
    constexpr inline StrongType&
    operator-=(StrongType const& o) noexcept { m_value -= o.m_value; return *this; }
    constexpr inline StrongType&
    operator*=(StrongType const& o) noexcept { m_value *= o.m_value; return *this; }
    constexpr inline StrongType&
    operator/=(StrongType const& o) noexcept { m_value /= o.m_value; return *this; }

    // pre increment
    constexpr inline StrongType&
    operator++() noexcept { ++m_value; return *this; }
    // post increment
    constexpr inline StrongType
    operator++(int) noexcept { StrongType tmp(*this); operator++(); return tmp; }

    // pre decrement
    constexpr inline StrongType&
    operator--() noexcept { --m_value; return *this; }
    // post decrement
    constexpr inline StrongType
    operator--(int) noexcept { StrongType tmp(*this); operator--(); return tmp; }

    operator T() const { return value(); }

    // Access the underlying value
    constexpr inline T
    value() const { return m_value; }

private:
    T m_value = InitValue;
};

using NodeId    = StrongType<unsigned, struct NodeId_, std::numeric_limits<unsigned>::max()>;
using PortIndex = StrongType<unsigned, struct PortIndex_, std::numeric_limits<unsigned>::max()>;
using PortId    = StrongType<unsigned, struct PortId_, std::numeric_limits<unsigned>::max()>;

using Position = QPointF;

enum PortType
{
    In = 0,
    Out,
    NoType
};

enum ModelPolicy
{
    /// Model is just a dummy and may be closed as soon as its
    /// parent model is closed
    DummyModel = 0,
    /// Model is active and should be kept alive if its parent model
    /// is closed (default)
    ActiveModel = 1
};

enum NodeIdPolicy
{
    /// Indictaes that the node id may be updated if it already exists
    UpdateNodeId = 0,
    /// Indicates that the node id should not be updated.
    KeepNodeId = 1
};

enum ExecutorType
{
    NoExecutor = 0,
    SequentialExecutor,
    ParallelExecutor,
    DefaultExecutor = 255
};

namespace detail
{

template <typename T>
struct InvalidValue
{
    constexpr static T get() { return std::numeric_limits<T>::max(); }
};

template <typename T, typename Tag, T InitVal>
struct InvalidValue<StrongType<T, Tag, InitVal>>
{
    using StrongTypeDef = StrongType<T, Tag, InitVal>;

    constexpr static StrongTypeDef get() { return StrongTypeDef{InitVal}; };
};

} // namespace detail

template<typename T>
constexpr inline T invalid() noexcept
{
    return detail::InvalidValue<T>::get();
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

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator+=(gt::ig::StrongType<T, Tag, InitVal> const& a,
           gt::ig::StrongType<T, Tag, InitVal> const& b) noexcept { return a += b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator-=(gt::ig::StrongType<T, Tag, InitVal> const& a,
           gt::ig::StrongType<T, Tag, InitVal> const& b) noexcept { return a -= b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator*=(gt::ig::StrongType<T, Tag, InitVal> const& a,
           gt::ig::StrongType<T, Tag, InitVal> const& b) noexcept{ return a *= b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator/=(gt::ig::StrongType<T, Tag, InitVal> const& a,
           gt::ig::StrongType<T, Tag, InitVal> const& b) noexcept { return a /= b; }

template <typename T, typename Tag, T InitVal>
inline gt::log::Stream&
operator<<(gt::log::Stream& s, gt::ig::StrongType<T, Tag, InitVal> const& t)
{
    return s << t.value();
}

#endif // GT_IGGLOBALS_H
