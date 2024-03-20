/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_GLOBALS_H
#define GT_INTELLI_GLOBALS_H

#include <gt_logging.h>
#include <gt_exceptions.h>

#include <chrono>
#include <utility>

#include <QPointF>
#include <QRegExp>

namespace intelli
{

class NodeData;

constexpr auto max_timeout = std::chrono::milliseconds::max();

/**
 * @brief Denotes the possible port types
 */
enum class PortType
{
    /// Input port
    In = 0,
    /// Output port
    Out,
    /// Undefined port type (most uses are invalid!)
    NoType
};

inline constexpr PortType invert(PortType type) noexcept
{
    switch (type)
    {
    case PortType::In:
        return PortType::Out;
    case PortType::Out:
        return PortType::In;
    default:
        return type;
    }
}

// Base class for typesafe type aliases
template <typename T, typename Tag, T InitValue = 0>
class StrongType
{
public:

    using value_type = T;

    constexpr inline explicit StrongType(T value = InitValue) noexcept :
        m_value(std::move(value))
    {}
    StrongType(StrongType const&) = default;
    StrongType(StrongType&&) = default;
    StrongType& operator=(StrongType const& value) = default;
    StrongType& operator=(StrongType&& value) = default;
    ~StrongType() = default;

    template<typename U>
    constexpr static StrongType fromValue(U value) { return StrongType{static_cast<T>(value)}; }

    // Overload comparison operators as needed
    constexpr inline bool
    operator==(StrongType const& other) const noexcept { return m_value == other.m_value; }
    constexpr inline bool
    operator!=(StrongType const& other) const noexcept { return !(*this == other); }

    // do not allow comparissions between different strong types
    template<typename U, typename UTag, U u>
    constexpr inline bool
    operator==(StrongType<U, UTag, u> const& other) const noexcept = delete;
    template<typename U, typename UTag, U u>
    constexpr inline bool
    operator!=(StrongType<U, UTag, u> const& other) const noexcept = delete;

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

    constexpr operator T() const { return value(); }

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

using NodeDataPtr  = std::shared_ptr<const NodeData>;

using NodeDataPtrList = std::vector<std::pair<PortIndex, NodeDataPtr>>;

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

/**
 * Connection identificator that stores
 * out `NodeId`, out `PortIndex`, in `NodeId`, in `PortIndex`
 */
struct ConnectionId
{
    constexpr ConnectionId(NodeId _outNode, PortId _outPort,
                           NodeId _inNode, PortId _inPort) :
        outNodeId(_outNode), outPort(_outPort),
        inNodeId(_inNode), inPort(_inPort)
    {}

    NodeId outNodeId;
    PortId outPort;
    NodeId inNodeId;
    PortId inPort;
    
    constexpr ConnectionId reversed() const noexcept
    {
        return { inNodeId, inPort, outNodeId, outPort };
    }

    constexpr NodeId node(PortType type) const
    {
        switch (type)
        {
        case PortType::In:
            return inNodeId;
        case PortType::Out:
            return outNodeId;
        case PortType::NoType:
            throw GTlabException(__FUNCTION__, "invalid port type!");
        }
    }

    constexpr PortId port(PortType type) const
    {
        switch (type)
        {
        case PortType::In:
            return inPort;
        case PortType::Out:
            return outPort;
        case PortType::NoType:
            throw GTlabException(__FUNCTION__, "invalid port type!");
        }
    }

    constexpr bool isValid() const noexcept
    {
        return inNodeId  != invalid<NodeId>() ||
               outNodeId != invalid<NodeId>() ||
               inPort    != invalid<PortId>() ||
               outPort   != invalid<PortId>();
    }
    // Overload comparison operators as needed
    constexpr inline bool operator==(ConnectionId const& o) const noexcept {
        return inNodeId == o.inNodeId &&
               inPort == o.inPort &&
               outNodeId == o.outNodeId &&
               outPort == o.outPort;
    }
    constexpr inline bool operator!=(ConnectionId const& o) const noexcept {
        return !(*this == o);
    }
};

//! Enum for GraphicsObject::Type value
enum class GraphicsItemType
{
    None = 0,
    Node,
    NodeEvalState,
    Connection
};

namespace detail
{

template <>
struct InvalidValue<ConnectionId>
{
    constexpr static ConnectionId get() {
        return ConnectionId{ NodeId{}, PortId{}, NodeId{}, PortId{} };
    }
};

} // namespace detail

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator+=(StrongType<T, Tag, InitVal> const& a,
           StrongType<T, Tag, InitVal> const& b) noexcept { return a += b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator-=(StrongType<T, Tag, InitVal> const& a,
           StrongType<T, Tag, InitVal> const& b) noexcept { return a -= b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator*=(StrongType<T, Tag, InitVal> const& a,
           StrongType<T, Tag, InitVal> const& b) noexcept{ return a *= b; }

template <typename T, typename Tag, T InitVal>
constexpr inline bool
operator/=(StrongType<T, Tag, InitVal> const& a,
           StrongType<T, Tag, InitVal> const& b) noexcept { return a /= b; }

#if 0
struct ConnectionIdHasher
{
    size_t operator()(ConnectionId const& c) const noexcept
    {
        using T = typename NodeId::value_type;
        static_assert(2 * sizeof(T) == sizeof(uint64_t), "was expecting 32bit uint");

        size_t hash = 0;
        hash  = (size_t)(c.outNodeId ^ c.outPort) << 32;
        hash += c.inNodeId ^ c.inPort;
        return hash;
    }
};

struct NodeIdHasher
{
    size_t operator()(NodeId nodeId) const noexcept
    {
        using T = typename NodeId::value_type;
        return std::hash<T>{}(nodeId.value());
    }
};
#endif

} // namespace intelli

namespace gt
{
namespace re
{

namespace intelli
{

inline QRegExp forClassNames()
{
    return QRegExp(R"(^([a-zA-Z_][a-zA-Z0-9_]*::)*[a-zA-Z_][a-zA-Z0-9_]*$)");
}

} // namespace intelli

} // namespace re

namespace log
{

template <typename T, typename Tag, T InitVal>
inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::StrongType<T, Tag, InitVal> const& t)
{
    return s << t.value();
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeConnection["
            << con.outNodeId << ":" << con.outPort << "/"
            << con.inNodeId  << ":" << con.inPort  << "]";
    }
    return s.doLogSpace();
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::PortType type)
{
    switch (type)
    {
    case intelli::PortType::In: return s << "PortType::In";
    case intelli::PortType::Out: return s << "PortType::Out";
    case intelli::PortType::NoType: return s << "PortType::NoType";
    }

    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "PortType::INVALID(" << type << ')';
    }
    return s.doLogSpace();
}

} // namespace log

} // namespace gt


#endif // GT_INTELLI_GLOBALS_H
