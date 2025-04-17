/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHCONNECTIONMODEL_H
#define GT_INTELLI_GRAPHCONNECTIONMODEL_H

#include <intelli/node.h>

#include <QPointer>

namespace intelli
{

namespace detail
{

template <bool Reversed, typename Container>
struct begin_iterator
{
    auto operator()(Container const& c) const { return c.begin(); }
};
template <typename Container>
struct begin_iterator<true, Container>
{
    auto operator()(Container const& c) const { return c.rbegin(); }
};
template <bool Reversed, typename Container>
struct end_iterator
{
    auto operator()(Container const& c) const { return c.end(); }
};
template <typename Container>
struct end_iterator<true, Container>
{
    auto operator()(Container const& c) const { return c.rend(); }
};

} // namespace detail;

/**
 * @brief Iterator wrapper class that can be used to alter the behavior
 * (e.g. return type) of iterators
 */
template <typename Iterator, typename Proxy>
class proxy_iterator
{
public:

    // currently only fwd iteration supported
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = typename std::iterator_traits<Iterator>::difference_type;
    using value_type        = typename Proxy::value_type;
    using reference         = typename Proxy::reference;
    using pointer           = typename Proxy::pointer;

    Iterator i{};  /// base iterater
    Proxy proxy{}; /// proxy object

    proxy_iterator() = default;
    explicit proxy_iterator(Iterator i_, Proxy p_ = {}) :
        i(std::move(i_)), proxy(std::move(p_))
    { proxy.init(i); }

    reference operator*() { return proxy.get(i); }
    pointer operator->() { return &(proxy.get(i)); }

    bool operator==(proxy_iterator const& o) const { return i == o.i; }
    bool operator!=(proxy_iterator const& o) const { return !(operator==(o)); }

    /// pre increment
    proxy_iterator& operator++()
    {
        proxy.advance(i);
        return *this;
    }
    /// post increment
    proxy_iterator operator++(int)
    {
        proxy_iterator cpy{i};
        operator++();
        return cpy;
    }
};

/**
 * @brief Default proxy object that yields `Value`.
 */
template <typename Value>
struct DefaultProxy
{
    using value_type = Value;
    using reference  = value_type&;
    using pointer    = value_type*;

    /// initializes the proxy
    template <typename Iter>
    void init(Iter&) {}

    /// returns the underlying value type of iterator
    template <typename Iter>
    reference get(Iter& i) { return *i; }

    /// advances the underlying iterator
    template <typename Iter>
    void advance(Iter& i) { ++i; }
};

/**
 * @brief Helper struct to instantiate a begin and end iterator over a
 * given range. Adds methods for accessing size and allows reverting iterator
 * directions if its supported
 */
template <typename Iterator, typename Proxy>
struct iterator_instantiator
{
    iterator_instantiator(Iterator b_ = {}, Iterator e_ = {}, Proxy p_ = {}) :
        b(std::move(b_)), e(std::move(e_)), p{std::move(p_)}
    {}

    using ProxyIter = proxy_iterator<Iterator, Proxy>;

    ProxyIter begin() const { return ProxyIter{ b, p }; }
    ProxyIter end() const { return ProxyIter{ e, p }; }

    bool empty() const { return begin() == end(); }
    size_t size() const { return std::distance(begin(), end()); }

    auto reverse() const {
        using reversed_type = typename Iterator::reversed_type;
        return iterator_instantiator<reversed_type, Proxy>{
            b.reverse(), e.reverse(), p
        };
    }

private:
    /// Iterator strategy
    Iterator b, e;
    /// Proxy object
    Proxy p;
};

/**
 * @brief Helper method that instantiates an iterable object and installs a
 * proxy object on the begin and end iterators.
 */
template <typename Proxy, typename Iterator>
inline auto makeProxy(Iterator begin, Iterator end, Proxy p = {})
{
    return iterator_instantiator<Iterator, Proxy>{
        std::move(begin), std::move(end), std::move(p)
    };
}

/**
 * @brief Helper method that instantiates an iterable object based on `t`'s
 * begin and end operator and installs a proxy object on these iterators.
 */
template <typename Proxy, typename T>
inline auto makeProxy(T const& t, Proxy p = {})
{
    return makeProxy(t.begin(), t.end(), std::move(p));
}

/**
 * @brief Helper method that instantiates an iterable object based on `t`'s
 * reverse begin and end operator. A default proxy object is installed that
 * yields the underlying value type.
 */
template <typename T>
inline auto makeReverseIter(T&& t)
{
    return makeProxy(t.rbegin(), t.rend(), DefaultProxy<typename std::decay_t<T>::value_type>{});
}

/**
 * @brief Helper struct that allows the use of for-range based loops and similar
 * constructs
 */
template <typename T>
struct iterable
{
    T b, e;

    T begin() const { return b; }
    T end() const { return b; }

    bool empty() const { return begin() == end(); }
    size_t size() const { return std::distance(begin(), end()); }
};

/**
 * @brief Helper method that instantiates an iterable object
 */
template <typename Iterator>
inline iterable<Iterator> makeIterable(Iterator begin, Iterator end)
{
    return {std::move(begin), std::move(end)};
}

/**
 * @brief Helper method that instantiates an iterable object based on `t`'s
 * begin and end operator.
 */
template <typename T>
inline iterable<T> makeIterable(T const& t)
{
    return makeIterable(t.begin(), t.end());
}

template <typename NodeId_t>
struct ConnectionDetail
{
    /// target node
    NodeId_t node;
    /// target port
    PortId port;
    /// source port
    PortId sourcePort;

    /**
     * @brief Creates an outgoing connection id. If `sourcePorts` refers to
     * a input-port the connection is reversed.
     * @param sourceNode Source (outgoing) node
     * @return Connection id
     */
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode) const
    {
        return { std::move(sourceNode), sourcePort, node, port };
    }
    /**
     * @brief Creates a normalized outgoing connection id. The port type
     * is used to normalize the direcction of the connection
     * @param sourceNode Source (outgoing) node
     * @param type Port type used for normalizing the connection
     * @return Connection id
     */
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode, PortType type) const
    {
        return type == PortType::In ? toConnection(std::move(sourceNode)).reversed() :
                                      toConnection(std::move(sourceNode));
    }

    /**
     * @brief Constructs an object from an outgoing connection
     * @param conId Source connection
     * @return Connection detail
     */
    static ConnectionDetail fromConnection(ConnectionId_t<NodeId_t> const& conId)
    {
        return { conId.inNodeId, conId.inPort, conId.outPort };
    }

    bool operator==(ConnectionDetail const& o) const
    {
        return node == o.node && port == o.port && sourcePort == o.sourcePort;
    }
    bool operator!=(ConnectionDetail const& o) const { return !(operator==(o)); }
};

template <typename NodeId_t>
struct ConnectionData
{
    using container_type = QVarLengthArray<ConnectionDetail<NodeId_t>, 10>;

    using const_forward_iter = typename container_type::const_iterator;
    using const_reverse_iter = typename container_type::const_reverse_iterator;

    /// pointer to node
    QPointer<Node> node;
    /// adjacency lists
    container_type predecessors = {}, successors = {};

    /**
     * @brief Returns the predecessors or successors depending on the port type
     * @param type Port type
     * @return Port vector
     */
    container_type& ports(PortType type)
    {
        assert(type != PortType::NoType);
        return (type == PortType::In) ? predecessors : successors;
    }
    container_type const& ports(PortType type) const
    {
        assert(type != PortType::NoType);
        return (type == PortType::In) ? predecessors : successors;
    }

    /**
     * @brief Equality operator
     * @param other Other entry
     * @return Is equal
     */
    bool operator==(ConnectionData const& other) const
    {
        auto compare = [](container_type const& list,
                          container_type const& other) {
            if (list.size() != other.size()) return false;

            return std::all_of(list.begin(), list.end(), [&other](auto& entry){
                return std::count(other.begin(), other.end(), entry) == 1;
            });
        };
        return node == other.node &&
               compare(predecessors, other.predecessors) &&
               compare(successors, other.successors);
    }
    bool operator!=(ConnectionData const& other) const { return !(*this == other); }

    /**
     * @brief The base_iterator struct. Defines a basic iterator for
     * `ConnectionData` that takes a strategy object to specialize the iterating
     * hebavior.
     */
    template <typename Strategy, bool Reversed = false>
    struct base_iterator
    {
        /// whether the iterator is reversed
        static constexpr bool IsReversed = Reversed;

        // std iterator API
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = ptrdiff_t;

        using value_type = ConnectionDetail<NodeId_t> const;
        using reference = value_type&;
        using pointer = value_type*;

        // helper types
        using reversed_type = base_iterator<Strategy, !Reversed>;
        using container_iterator =
            std::conditional_t<Reversed, const_reverse_iter,
                                         const_forward_iter>;

        /// default constructor (is invalid by default)
        base_iterator() { assert(!isValid()); }
        /// constructor to instantiate a specific iterator
        base_iterator(ConnectionData const* ptr_,
                      container_iterator begin_,
                      container_iterator end_,
                      PortType type_,
                      Strategy strategy_ = {}) :
            ptr(ptr_),
            iter(begin_),
            end(end_),
            type(type_),
            strategy(std::move(strategy_))
        {
            // initialize iterator to point to valid position if possible
            if (iter == end || !strategy.isValid(*this)) operator++();
        }

        /**
         * @brief Reverses the iterator, i.e. such that it iterates backwards
         * through all lists.
         * NOTE: Should be called on a freshly instantiated object
         * otherwise the reversed iterator may skip over some entries.
         * @return Reversed iterator
         */
        reversed_type reverse() const
        {
            if (!ptr) return {};

            auto& p = ptr->ports(type);
            reversed_type riter{
                ptr,
                detail::begin_iterator<!Reversed, container_type>{}(p),
                detail::end_iterator<!Reversed, container_type>{}(p),
                type,
                strategy
            };
            return riter;
        }

        /// helper method that yields whether the iterator is safe to
        /// dereference
        bool isValid() const
        {
            return ptr &&
                   iter != end &&
                   type != PortType::NoType &&
                   strategy.isValid(*this);
        }

        /// Dereference operator. The iterator must be valid
        reference operator*()
        {
            assert(isValid() && "accessed invalid iterator!");
            return *iter;
        }
        /// Pointer operator
        pointer operator->() { return &(operator*()); }

        /// Equality operator
        bool operator==(base_iterator const& o) const
        {
            return (!isValid() && !o.ptr) || // `this` is at end, other is "empty"
                   (!ptr && !o.isValid()) || // `this` is "empty", other is at end
                   // object is identical
                   (ptr == o.ptr &&
                    iter == o.iter &&
                    type == o.type &&
                    strategy.isEqualTo(o.strategy));
        }
        /// Inequality operator
        bool operator!=(base_iterator const& o) const { return !operator==(o); }

        /// Pre-increment operator
        base_iterator& operator++()
        {
            strategy.advance(*this);
            return *this;
        }

        /// pointer to connection data
        ConnectionData const* ptr{};
        /// current iterator of underlying list
        container_iterator iter{};
        /// end iterator of underlying list
        container_iterator end{};
        /// which side the iterator is pointing to
        PortType type{PortType::NoType};
        /// strategy object to alter iterator behvaior
        Strategy strategy{};
    };

    /**
     * @brief The IterateOneSide struct. Strategy to iterate only over
     * predecessors OR successors.
     */
    struct IterateOneSide
    {
        template <typename Base>
        bool isValid(Base const& i) const { return true; }

        bool isEqualTo(IterateOneSide const& o) const { return true; }

        template <typename Base>
        void advance(Base& i)
        {
            if (i.iter != i.end) ++i.iter;
        }
    };

    /**
     * @brief The IterateBothSides struct. Strategy to iterate over
     * predecessors AND successors.
     */
    struct IterateBothSides
    {
        template <typename Base>
        bool isValid(Base const& i) const { return true; }

        bool isEqualTo(IterateBothSides const& o) const { return true; }

        template <typename Base>
        void advance(Base& i)
        {
            if (i.iter != i.end) ++i.iter;
            // finished iterating input side -> switch to output side
            if (i.iter == i.end && i.type == PortType::In)
            {
                auto& p = i.ptr->ports(PortType::Out);
                i.iter = detail::begin_iterator<Base::IsReversed, container_type>{}(p);
                i.end  = detail::end_iterator<Base::IsReversed, container_type>{}(p);
                i.type = PortType::Out;
            }
        }
    };

    /**
     * @brief The IterateByPortOneSide struct. Strategy to iterate over
     * a specific port in only predecessors OR successors list.
     */
    struct IterateByPortOneSide
    {
        PortId portId{};

        template <typename Base>
        bool isValid(Base const& i) const
        {
            return portId == i.iter->sourcePort;
        }

        bool isEqualTo(IterateByPortOneSide const& o) const
        {
            return portId == o.portId;
        }

        template <typename Base>
        void advance(Base& i)
        {
            while (i.iter != i.end)
            {
                ++i.iter;
                // valid port found
                if (i.iter != i.end && i.iter->sourcePort == portId) break;
            }
        }
    };

    /**
     * @brief The IterateByPortBothSides struct. Strategy to iterate over
     * a specific port in both predecessors AND successors lists.
     */
    struct IterateByPortBothSides
    {
        PortId portId{};

        template <typename Base>
        bool isValid(Base const& i) const
        {
            return portId == i.iter->sourcePort;
        }

        bool isEqualTo(IterateByPortBothSides const& o) const
        {
            return portId == o.portId;
        }

        template <typename Base>
        void advance(Base& i)
        {
            while (true)
            {
                // finished iterating one side
                if (i.iter == i.end)
                {
                    // both sides searched
                    if (i.type != PortType::In) break;

                    // switch to output side
                    auto& p = i.ptr->ports(PortType::Out);
                    i.iter = detail::begin_iterator<Base::IsReversed, container_type>{}(p);
                    i.end  = detail::end_iterator<Base::IsReversed, container_type>{}(p);
                    i.type = PortType::Out;
                }
                else
                {
                    ++i.iter;
                }
                // valid port found
                if (i.iter != i.end && i.iter->sourcePort == portId) break;
            }
        }
    };

    /// Proxy object that yields `ConnectionDetail<NodeId_t>`.
    using ConnectionDetailProxy = DefaultProxy<ConnectionDetail<NodeId_t> const>;

    /**
     * @brief Can be used to iterate over all predecessors OR successors
     * depending on the given port type.
     * @param type Port type, denoting whether to iterate over predecessors or
     * successors
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterate(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateOneSide>, ConnectionDetailProxy>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }
    /**
     * @brief Can be used to iterate over all predecessors AND successors.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterate() const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateBothSides>, ConnectionDetailProxy>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }
    /**
     * @brief Can be used to iterate over all predecessors OR successors
     * that start/end at the given port depending on the given port type.
     * @param portId Port id
     * @param type Port type, denoting whether to iterate over predecessors or
     * successors
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterate(PortId portId, PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateByPortOneSide>, ConnectionDetailProxy>{
            { this, p.begin(), p.end(), type, { portId } }
        };
    }
    /**
     * @brief Can be used to iterate over all predecessors and successors
     * that start/end at the given port. Will iterate over all input and output
     * ports internally.
     * @param portId Port id
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterate(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateByPortBothSides>, ConnectionDetailProxy>{
            { this, p.begin(), p.end(), PortType::In, { portId } }
        };
    }

    /**
     * @brief Proxy object that yields `ConnectionId_t<NodeId_t>`.
     */
    struct ConnectionIdProxy
    {
        using value_type = ConnectionId_t<NodeId_t> const;
        using reference  = value_type;
        using pointer    = value_type;

        template <typename Iter>
        void init(Iter& i) { }

        template <typename Iter>
        reference get(Iter& i)
        {
            return (*i).toConnection(get_node_id<NodeId_t>{}(i.ptr->node), i.type);
        }

        template <typename Iter>
        void advance(Iter& i) { ++i; }
    };

    /**
     * @brief Can be used to iterate over all ingoing OR outgoing connections
     * depending on the given port type.
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnections(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateOneSide>, ConnectionIdProxy>{
            { this, p.begin(), p.end(), type }
        };
    }
    /**
     * @brief Can be used to iterate over all ingoing AND outgoing connections.
     * Will iterate over all input and output ports internally.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnections() const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateBothSides>, ConnectionIdProxy>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }
    /**
     * @brief Can be used to iterate over all ingoing or outgoing connections
     * that start/end at the given port depending on the given port type.
     * @param portId Port id
     * @param type Port type, denoting whether to iterate over predecessors or
     * successors
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnections(PortId portId, PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateByPortOneSide>, ConnectionIdProxy>{
            { this, p.begin(), p.end(), type, { portId } }
        };
    }
    /**
     * @brief Can be used to iterate over all connections that either end or
     * start at the given port id.
     * @param portId Port id
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnections(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateByPortBothSides>, ConnectionIdProxy>{
            { this, p.begin(), p.end(), PortType::In, IterateByPortBothSides{ portId } }
        };
    }

    /**
     * @brief Proxy object that yields `NodeId_t`.
     */
    struct NodeIdProxy
    {
        using value_type = NodeId_t const;
        using reference  = value_type&;
        using pointer    = value_type*;

        template <typename Iter>
        void init(Iter&) {}

        template <typename Iter>
        reference get(Iter& i) { return (*i).node; }

        template <typename Iter>
        void advance(Iter& i) { ++i; }
    };

    /**
     * @brief Can be used to iterate over all nodes connected to either the
     * input or output side depending on the given port type. May contain
     * duplicates.
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateNodes(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateOneSide>, NodeIdProxy>{
            { this, p.begin(), p.end(), type }
        };
    }
    /**
     * @brief Can be used to iterate over all connetced nodes. May contain
     * duplicates.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateNodes() const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateBothSides>, NodeIdProxy>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }
    /**
     * @brief Can be used to iterate over all nodes that are connected to the
     * given port id at either the input or output side depending on the given
     * port type. May contain duplicates.
     * @param portId Port id
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateNodes(PortId portId, PortType type) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateByPortOneSide>, NodeIdProxy>{
            { this, p.begin(), p.end(), PortType::In, { portId } }
        };
    }
    /**
     * @brief Can be used to iterate over all nodes that are connected to the
     * given port id. Will iterate over all input and output ports internally.
     * May contain duplicates.
     * @param portId Port id
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateNodes(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateByPortBothSides>, NodeIdProxy>{
            { this, p.begin(), p.end(), PortType::In, { portId } }
        };
    }

    /**
     * @brief Proxy object that yields `NodeId_t`. Keeps track of visited
     * nodes.
     */
    struct UniqueNodeIdProxy
    {
        using value_type = NodeId_t const;
        using reference  = value_type&;
        using pointer    = value_type*;

        // cache to keep track of visited nodes
        QVarLengthArray<NodeId_t, 10> visited;

        template <typename Iter>
        void init(Iter& i)
        {
            if (i.isValid()) visited << get(i);
        }

        template <typename Iter>
        reference get(Iter& i) { return (*i).node; }

        template <typename Iter>
        void advance(Iter& i)
        {
            while (i.isValid() && visited.contains(get(i))) ++i;
            if (i.isValid()) visited << get(i);
        }
    };

    /**
     * @brief Can be used to iterate over all nodes connected to either the
     * inside or outside depending on the given port type.
     * Only yields unqiue entries.
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateUniqueNodes(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateOneSide>, UniqueNodeIdProxy>{
            { this, p.begin(), p.end(), type }
        };
    }
    /**
     * @brief Can be used to iterate over all connetced nodes.
     * Only yields unqiue entries.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateUniqueNodes() const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateBothSides>, UniqueNodeIdProxy>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }
    /**
     * @brief Can be used to iterate over all nodes that are connected to the
     * given port id at either the input or output side depending on the given
     * port type.. Will iterate over all input and output ports internally.
     * Only yields unqiue entries.
     * @param portId Port id
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateUniqueNodes(PortId portId, PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<base_iterator<IterateByPortOneSide>, UniqueNodeIdProxy>{
            { this, p.begin(), p.end(), type, { portId } }
        };
    }
    /**
     * @brief Can be used to iterate over all nodes that are connected to the
     * given port id. Will iterate over all input and output ports internally.
     * Only yields unqiue entries.
     * @param portId Port id
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateUniqueNodes(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<base_iterator<IterateByPortBothSides>, UniqueNodeIdProxy>{
            { this, p.begin(), p.end(), PortType::In, { portId } }
        };
    }

    bool hasConnections(PortType type = PortType::NoType) const
    {
        return type == PortType::NoType ?
                   !iterate().empty() :
                   !iterate(type).empty();
    }

    bool hasConnections(PortId portId, PortType type = PortType::NoType) const
    {
        return type == PortType::NoType ?
                   !iterate(portId).empty() :
                   !iterate(portId, type).empty();
    }
};

/// directed acyclic graph representing connections and nodes
template <typename NodeId_t>
class ConnectionModel_t
{
public:

    using key_type = NodeId_t;
    using value_type = ConnectionData<NodeId_t>;
    using data_type = QHash<key_type, value_type>;
    using size_type = typename data_type::size_type;

    using iterator = typename data_type::iterator;
    using const_iterator = typename data_type::const_iterator;
    using key_value_iterator = typename data_type::key_value_iterator;
    using const_key_value_iterator = typename data_type::const_key_value_iterator;

    //**** custom ****//

    /**
     * @brief Inserts the node for the given key
     * @param key Id of node
     * @param node Node pointer
     * @return iterator
     */
    iterator insert(key_type const& key, Node* node)
    {
        assert(node);
        return m_data.insert(key, { node });
    }

    /**
     * @brief Attempts to access the node for the given key. If the key is not
     * present, nullptr is returned.
     * @param key Node Id
     * @return Node pointer (may be null)
     */
    Node* node(NodeId_t const& key)
    {
        auto iter = find(key);
        return (iter == end()) ? nullptr : iter->node;
    }
    Node const* node(NodeId_t const& key) const
    {
        auto iter = find(key);
        return (iter == end()) ? nullptr : iter->node;
    }

    /**
     * @brief Returns whether the node has out-/ingoing connections depending
     * on `type` parameter.
     * @param nodeId Node id to look up
     * @param type Whether both (PortType::NoType) or only input (PortType::In)
     * or output (PortType::Out) connections should be counted.
     * @return Whether the node has out-/ingoing connections. Returns false
     * if node was not found
     */
    bool hasConnections(NodeId nodeId, PortType type = PortType::NoType) const
    {
        return type == PortType::NoType ?
                   !iterate(nodeId).empty() :
                   !iterate(nodeId, type).empty();
    }
    /**
     * @brief Returns whether any node has out- or ingoing connections
     * @return Whether any node has out- or ingoing connections
     */
    bool hasConnections() const
    {
        return std::any_of(m_data.keyBegin(),
                           m_data.keyEnd(),
                           [this](NodeId nodeId){
            return hasConnections(nodeId);
        });
    }

    /// Proxy that yields `NodeId_t`
    using NodeIdProxy = DefaultProxy<NodeId_t const>;

    /**
     * @brief Iterates over all node ids
     */
    auto iterateNodeIds() const
    {
        using Iter = decltype(m_data.keyBegin());
        return iterator_instantiator<Iter, NodeIdProxy>{
            m_data.keyBegin(), m_data.keyEnd()
        };
    }

    /**
     * @brief Proxy object that yields `Node*`.
     */
    template <typename T>
    struct NodeProxy
    {
        using value_type = T*;
        using reference  = value_type;
        using pointer    = value_type;

        template <typename Iter>
        void init(Iter&) {}

        template <typename Iter>
        reference get(Iter& i) { return (*i).node; }

        template <typename Iter>
        void advance(Iter& i) { ++i; }
    };

    /**
     * @brief Iterates over all nodes (pointers)
     */
    auto iterateNodes() const
    {
        return iterator_instantiator<const_iterator, NodeProxy<Node const>>{begin(), end()};
    }
    auto iterateNodes()
    {
        return iterator_instantiator<const_iterator, NodeProxy<Node>>{begin(), end()};
    }

    /**
     * @brief Convenience function. Exposes `iterate` method of the entry
     * denoted by `nodeId`. If entry was not found an empty range is returned.
     * @param nodeId Target entry
     * @param args Forwarding arguments the desired `iterate` call.
     */
    template <typename... Args>
    auto iterate(NodeId_t const& nodeId, Args&&... args) const
    {
        auto iter = find(nodeId);
        if (iter == end())
        {
            using R = decltype(iter->iterate(std::forward<Args>(args)...));
            return R{};
        }
        return iter->iterate(std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience function. Exposes `iterateConnections` method of the
     * entry denoted by `nodeId`. If entry was not found an empty range is
     * returned.
     * @param nodeId Target entry
     * @param args Forwarding arguments the desired `iterateConnections` call.
     */
    template <typename... Args>
    auto iterateConnections(NodeId_t const& nodeId, Args&&... args) const
    {
        auto iter = find(nodeId);
        if (iter == end())
        {
            using R = decltype(iter->iterateConnections(std::forward<Args>(args)...));
            return R{};
        }
        return iter->iterateConnections(std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience function. Exposes `iterateNodes` method of the
     * entry denoted by `nodeId`. If entry was not found an empty range is
     * returned.
     * @param nodeId Target entry
     * @param args Forwarding arguments the desired `iterateNodes` call.
     */
    template <typename... Args>
    auto iterateNodes(NodeId_t const& nodeId, Args&&... args) const
    {
        auto iter = find(nodeId);
        if (iter == end())
        {
            using R = decltype(iter->iterateNodes(std::forward<Args>(args)...));
            return R{};
        }
        return iter->iterateNodes(std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience function. Exposes `iterateUniqueNodes` method of the
     * entry denoted by `nodeId`. If entry was not found an empty range is
     * returned.
     * @param nodeId Target entry
     * @param args Forwarding arguments the desired `iterateUniqueNodes` call.
     */
    template <typename... Args>
    auto iterateUniqueNodes(NodeId_t const& nodeId, Args&&... args) const
    {
        auto iter = find(nodeId);
        if (iter == end())
        {
            using R = decltype(iter->iterateUniqueNodes(std::forward<Args>(args)...));
            return R{};
        }
        return iter->iterateUniqueNodes(std::forward<Args>(args)...);
    }

    //**** QHash ****//
    iterator insert(key_type const& key, value_type const& value) { return m_data.insert(key, value); }
    void insert(ConnectionModel_t const& other) { m_data.insert(other.m_data); }

    void clear() { m_data.clear(); }

    iterator erase(iterator iter) { return m_data.erase(iter); }
    iterator erase(const_iterator iter) { return m_data.erase(iter); }

    iterator find(key_type const& key) { return m_data.find(key); }
    const_iterator find(key_type const& key) const { return m_data.find(key); }

    bool contains(key_type const& key) const { return m_data.contains(key); }

    bool empty() const { return m_data.empty(); }
    bool isEmpty() const { return m_data.empty(); }

    size_type size() const { return m_data.size(); }
    size_type length() const { return m_data.size(); }

    value_type value(key_type const& key) const { return m_data.value(key); }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    key_value_iterator keyValueBegin() { return m_data.keyValueBegin(); }
    key_value_iterator keyValueEnd() { return m_data.keyValueEnd(); }
    const_key_value_iterator keyValueBegin() const { return m_data.keyValueBegin(); }
    const_key_value_iterator keyValueEnd() const { return m_data.keyValueEnd(); }

    bool operator==(ConnectionModel_t const& other) const
    {
        if (size() != other.size()) return false;

        auto iter = keyValueBegin();
        auto end  = keyValueEnd();
        for (; iter != end; ++iter)
        {
            auto oIter = other.find(std::get<0>(*iter));
            if (oIter == other.end()) return false;
            if (*oIter != std::get<1>(*iter)) return false;
        }

        return m_data == other.m_data;

    }
    bool operator!=(ConnectionModel_t const& other) const { return !(*this == other); }

private:

    /// data
    data_type m_data;
};

// definitions
using ConnectionModel  = ConnectionModel_t<NodeId>;
using GlobalConnectionModel = ConnectionModel_t<NodeUuid>;

// operators
template <typename T>
inline bool operator==(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b)
{
    return a.node == b.node && a.port == b.port && a.sourcePort == b.sourcePort;
}

template <typename T>
inline bool operator!=(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b) { return !(a == b); }

} // namespace intelli;

#endif // GT_INTELLI_GRAPHCONNECTIONMODEL_H
