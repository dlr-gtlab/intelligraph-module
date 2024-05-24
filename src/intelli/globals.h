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
#include <stdlib.h>

#include <QPointF>
#include <QRegExp>

namespace intelli
{

class NodeData;

/**
 * @brief Quantizes `point` so that it is a multiple of `stepSize`.
 * Example:
 *     quantize(QPointF(42.4,9.75), 5) -> QPoint(40, 10)
 * @param point Point to quantize
 * @param stepSize Step size to use for quantization
 * @return quantized point
 */
inline QPoint quantize(QPointF point, int stepSize)
{
    auto divX = std::div(point.x(), stepSize);
    auto divY = std::div(point.y(), stepSize);
    double x = divX.rem;
    double y = divY.rem;

    double stepHalf = 0.5 * stepSize;
    divX.quot += x > stepHalf ? 1 : x < -stepHalf ? -1 : 0;
    divY.quot += y > stepHalf ? 1 : y < -stepHalf ? -1 : 0;

    return QPoint{divX.quot * stepSize, divY.quot * stepSize};
};

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

    constexpr bool isValid() const noexcept;

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

using NodeUuid = QString;
using NodeId    = StrongType<unsigned, struct NodeId_, std::numeric_limits<unsigned>::max()>;
using PortIndex = StrongType<unsigned, struct PortIndex_, std::numeric_limits<unsigned>::max()>;
using PortId    = StrongType<unsigned, struct PortId_, std::numeric_limits<unsigned>::max()>;

using Position = QPointF;

using NodeDataPtr = std::shared_ptr<const NodeData>;

using NodeDataPtrList = std::vector<std::pair<PortId, NodeDataPtr>>;

using TypeName = QString;

using TypeId = QString;
using TypeIdList = QStringList;

namespace detail
{

template<typename T>
struct InvalidValue
{
    constexpr static T get() { return std::numeric_limits<T>::max(); }
};

template<typename T, typename Tag, T InitVal>
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
    constexpr ConnectionId() {};

    constexpr ConnectionId(NodeId _outNode, PortId _outPort,
                           NodeId _inNode, PortId _inPort) :
        outNodeId(_outNode), outPort(_outPort),
        inNodeId(_inNode), inPort(_inPort)
    {}

    NodeId outNodeId;
    PortId outPort;
    NodeId inNodeId;
    PortId inPort;

    /// Reverses the node and port ids, such that the out node becomes the
    /// in node
    constexpr void reverse() noexcept
    {
        *this = reversed();
    }

    /// Returns a new connection that has its node and port ids reversed
    constexpr ConnectionId reversed() const noexcept
    {
        return { inNodeId, inPort, outNodeId, outPort };
    }

    /// Returns the node id associated with the port type
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

    /// Returns the port id associated with the port type
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

    /// Whether this connection is valid (i.e. contains only valid node
    /// and port ids)
    constexpr bool isValid() const noexcept
    {
        return inNodeId  != invalid<NodeId>() &&
               outNodeId != invalid<NodeId>() &&
               inPort    != invalid<PortId>() &&
               outPort   != invalid<PortId>();
    }

    /// Whether this connection is a draft (i.e. one side is valid)
    constexpr bool isDraft() const noexcept
    {
        return draftType() != PortType::NoType;
    }

    /// Returns which side of the draft connection is valid
    constexpr PortType draftType() const noexcept
    {
        if (outNodeId == invalid<NodeId>() && outPort == invalid<PortId>() &&
            inNodeId  != invalid<NodeId>() && inPort  != invalid<PortId>())
        {
            return PortType::In;
        }
        if (inNodeId  == invalid<NodeId>() && inPort  == invalid<PortId>() &&
            outNodeId != invalid<NodeId>() && outPort != invalid<PortId>())
        {
            return PortType::Out;
        }
        return PortType::NoType;
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

/// Enum for GraphicsObject::Type value
enum class GraphicsItemType
{
    None = 0,
    Node,
    NodeEvalState,
    Connection
};

/// Enum indicating the evauation state of a node
enum class NodeEvalState
{
    Invalid = 0,
    Outdated,
    Evaluating,
    Paused,
    Valid
};

// Enum indicating the data state of a node port
enum class PortDataState
{
    /// Port data is outdated
    Outdated = 0,
    /// Port data is valid and up-to-date
    Valid,
};

struct NodeDataSet
{
    NodeDataSet(std::nullptr_t) :
        ptr(nullptr), state(PortDataState::Outdated)
    {}
    NodeDataSet(NodeDataPtr _data = {}) :
        ptr(std::move(_data)), state(PortDataState::Valid)
    {}
    template <typename T>
    NodeDataSet(std::shared_ptr<T> _data) :
        ptr(std::move(_data)), state(PortDataState::Valid)
    {}

    /// actual node data
    NodeDataPtr ptr;
    /// data state
    PortDataState state;

    operator NodeDataPtr&() & { return ptr; }
    operator NodeDataPtr() && { return std::move(ptr); }
    operator NodeDataPtr const&() const& { return ptr; }

    template <typename T>
    inline auto value() const noexcept { return qobject_pointer_cast<T const>(ptr); }
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

template <typename T, typename Tag, T InitValue>
constexpr inline bool StrongType<T, Tag, InitValue>::isValid() const noexcept
{
    return *this != invalid<StrongType<T, Tag, InitValue>>();
}

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
            << "Connection["
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
            << "PortType::INVALID(" << (int)type << ')';
    }
    return s.doLogSpace();
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::NodeEvalState state)
{
    switch (state)
    {
    case intelli::NodeEvalState::Evaluating: return s << "NodeEvalState::Evaluating";
    case intelli::NodeEvalState::Invalid: return s << "NodeEvalState::Invalid";
    case intelli::NodeEvalState::Outdated: return s << "NodeEvalState::Outdated";
    case intelli::NodeEvalState::Paused: return s << "NodeEvalState::Paused";
    case intelli::NodeEvalState::Valid: return s << "NodeEvalState::Valid";
    }

    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeEvalState::INVALID(" << (int)state << ')';
    }
    return s.doLogSpace();
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::PortDataState state)
{
    switch (state)
    {
    case intelli::PortDataState::Outdated: return s << "PortDataState::Outdated";
    case intelli::PortDataState::Valid: return s << "PortDataState::Valid";
    }

    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "PortDataState::INVALID(" << (int)state << ')';
    }
    return s.doLogSpace();
}

} // namespace log

} // namespace gt


#endif // GT_INTELLI_GLOBALS_H
