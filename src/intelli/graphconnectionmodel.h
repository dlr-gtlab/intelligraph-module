/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 27.8.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
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

    using iterator_category = typename Iterator::iterator_category;
    using difference_type   = typename Iterator::difference_type;
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
    pointer operator->() { return &(*this); }

    bool operator==(proxy_iterator const& o) const { return i == o.i; }
    bool operator!=(proxy_iterator const& o) const { return !(operator==(o)); }

    /// pre increment
    proxy_iterator& operator++()
    {
        proxy.next(i);
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
     * @brief Base iterator class that accepts a "strategy" object to operate on
     */
    template <typename Strategy>
    class base_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = ptrdiff_t;
        using value_type = typename Strategy::value_type;
        using pointer    = typename Strategy::pointer;
        using reference  = typename Strategy::reference;

        Strategy s{}; /// iteration strategy

        base_iterator() = default;
        explicit base_iterator(Strategy data_) : s(std::move(data_)) { }

        reference operator*() { return s.get(); }
        pointer operator->() { return &s.get(); }

        bool operator==(base_iterator const& o) const { return s == o.s; }
        bool operator!=(base_iterator const& o) const { return !(operator==(o)); }

        /// pre increment
        base_iterator& operator++()
        {
            s.next();
            return *this;
        }
        /// post increment
        base_iterator operator++(int)
        {
            base_iterator cpy{s};
            operator++();
            return cpy;
        }
    };

    /**
     * @brief Helper struct to instantiate a begin and end iterator on
     */
    template <typename Strategy, typename Proxy>
    struct iterator_instantiator
    {
        iterator_instantiator(Strategy b_ = {}, Strategy e_ = {}) :
            b(std::move(b_)), e(std::move(e_))
        {}

        using BaseIter = base_iterator<Strategy>;
        using ProxyIter = proxy_iterator<BaseIter, Proxy>;

        ProxyIter begin() const { return ProxyIter{ BaseIter{b} }; }
        ProxyIter end() const { return ProxyIter{ BaseIter{e} }; }
        bool empty() const { return begin() == end(); }
        size_t size() const { return std::distance(begin(), end()); }

        auto reverse() const {
            using reversed_type = typename Strategy::reversed_type;
            return iterator_instantiator<reversed_type, Proxy>{
                b.reverse(), e.reverse()
            };
        }

    private:
        Strategy b, e;
    };

    /**
     * @brief Helper object to iterate only over all predecessors OR successors
     * of this object
     */
    template <bool Reversed = false>
    struct iterate_one_side
    {
        using reversed_type = iterate_one_side<!Reversed>;

        using value_type = ConnectionDetail<NodeId_t> const;
        using reference = value_type&;
        using pointer = value_type*;

        using container_iterator =
            std::conditional_t<Reversed, const_reverse_iter, const_forward_iter>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator end{};
        PortType type{PortType::NoType};

        iterate_one_side() { assert(!isValid()); }
        iterate_one_side(ConnectionData const* ptr_,
                         container_iterator begin_,
                         container_iterator end_,
                         PortType type_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_)
        { }

        reversed_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<!Reversed, container_type>{}(p),
                detail::end_iterator<!Reversed, container_type>{}(p),
                type
            };
        }

        bool isValid() const
        {
            return ptr && type != PortType::NoType && iter != end;
        }

        reference get() const
        {
            assert(isValid() && "accessed invalid iterator! (iterate_one_side)");
            return *iter;
        }

        bool operator==(iterate_one_side const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type);
        }

        void next()
        {
            if (iter != end) ++iter;
        }
    };

    /**
     * @brief Helper object to iterate only over all predecessors AND successors
     * of this object
     */
    template <bool Reversed = false>
    struct iterate_both_sides
    {
        using reversed_type = iterate_both_sides<!Reversed>;

        using value_type = ConnectionDetail<NodeId_t> const;
        using reference = value_type&;
        using pointer = value_type*;

        using container_iterator =
            std::conditional_t<Reversed, const_reverse_iter, const_forward_iter>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator end{};
        PortType type{PortType::NoType};

        iterate_both_sides() { assert(!isValid()); }
        iterate_both_sides(ConnectionData const* ptr_,
                           container_iterator begin_,
                           container_iterator end_,
                           PortType type_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_)
        {
            // find initial valid position
            if (iter == end) next();
        }

        reversed_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<!Reversed, container_type>{}(p),
                detail::end_iterator<!Reversed, container_type>{}(p),
                type
            };
        }

        bool isValid() const
        {
            return ptr && type != PortType::NoType && iter != end;
        }

        reference get() const
        {
            assert(isValid() && "accessed invalid iterator! (iterate_both_sides)");
            return *iter;
        }

        bool operator==(iterate_both_sides const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type);
        }

        void next()
        {
            if (iter != end) ++iter;
            // finsihed iterating input side -> switch to output side
            if (iter == end && type == PortType::In)
            {
                auto& p = ptr->ports(PortType::Out);
                iter = detail::begin_iterator<Reversed, container_type>{}(p);
                end  = detail::end_iterator<Reversed, container_type>{}(p);
                type = PortType::Out;
            }
        }
    };

    /**
     * @brief Helper object to iterate only over all predecessors and successors
     * that have the specified `portId` as the source port
     */
    template <bool Reversed = false>
    struct iterate_by_port
    {
        using reversed_type = iterate_by_port<!Reversed>;

        using value_type = ConnectionDetail<NodeId_t> const;
        using reference = value_type&;
        using pointer = value_type*;

        using container_iterator =
            std::conditional_t<Reversed, const_reverse_iter, const_forward_iter>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator end{};
        PortType type{PortType::NoType};
        PortId portId{};

        iterate_by_port() { assert(!isValid()); }
        iterate_by_port(ConnectionData const* ptr_,
                        container_iterator begin_,
                        container_iterator end_,
                        PortType type_,
                        PortId portId_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_), portId(portId_)
        {
            // find initial valid position
            if (iter == end || iter->sourcePort != portId) next();
        }

        reversed_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<!Reversed, container_type>{}(p),
                detail::end_iterator<!Reversed, container_type>{}(p),
                type,
                portId
            };
        }

        bool isValid() const
        {
            return ptr && type != PortType::NoType && iter != end ;
        }

        reference get() const
        {
            assert(isValid() && "accessed invalid iterator! (iterate_by_port)");
            return *iter;
        }

        bool operator==(iterate_by_port const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type && portId == o.portId);
        }

        void next()
        {
            while (true)
            {
                // finished iterating one side
                if (iter == end)
                {
                    // both sides searched
                    if (type != PortType::In) break;

                    // switch to output side
                    auto& p = ptr->ports(PortType::Out);
                    iter = detail::begin_iterator<Reversed, container_type>{}(p);
                    end  = detail::end_iterator<Reversed, container_type>{}(p);
                    type = PortType::Out;
                }
                else
                {
                    ++iter;
                }
                // valid port found
                if (iter != end && iter->sourcePort == portId) break;
            }
        }
    };

    /**
     * @brief Proxy object that yields `ConnectionDetail<NodeId_t>`.
     */
    struct DefaultProxy
    {
        using value_type = ConnectionDetail<NodeId_t> const;
        using reference  = value_type&;
        using pointer    = value_type*;

        template <typename Iter>
        void init(Iter&) {}
        template <typename Iter>
        reference get(Iter& i) { return *i; }
        template <typename Iter>
        void next(Iter& i) { ++i; }
    };

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
        return iterator_instantiator<iterate_one_side<>, DefaultProxy>{
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
        return iterator_instantiator<iterate_both_sides<>, DefaultProxy>{
            { this, p.begin(), p.end(), PortType::In }
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
        return iterator_instantiator<iterate_by_port<>, DefaultProxy>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
    }

    /**
     * @brief Proxy object that yields `ConnectionId_t<NodeId_t>`.
     */
    struct ConnectionProxy
    {
        using value_type = ConnectionId_t<NodeId_t> const;
        using reference  = value_type;
        using pointer    = value_type;

        template <typename Iter>
        void init(Iter& i) { }

        template <typename Iter>
        reference get(Iter& i)
        {
            return i->toConnection(get_node_id<NodeId_t>{}(i.s.ptr->node), i.s.type);
        }

        template <typename Iter>
        void next(Iter& i) { ++i; }
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
        return iterator_instantiator<iterate_one_side<>, ConnectionProxy>{
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
        return iterator_instantiator<iterate_both_sides<>, ConnectionProxy>{
            { this, p.begin(), p.end(), PortType::In }
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
        return iterator_instantiator<iterate_by_port<>, ConnectionProxy>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
    }

    /**
     * @brief Proxy object that yields `NodeId_t`.
     */
    struct NodeProxy
    {
        using value_type = NodeId_t const;
        using reference  = value_type&;
        using pointer    = value_type*;

        template <typename Iter>
        void init(Iter&) {}

        template <typename Iter>
        reference get(Iter& i) { return i->node; }

        template <typename Iter>
        void next(Iter& i) { ++i; }
    };

    /**
     * @brief Can be used to iterate over all nodes connected to either the
     * inside or outside depending on the given port type. May contain
     * duplicates.
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateNodes(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<iterate_one_side<>, NodeProxy>{
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
        return iterator_instantiator<iterate_both_sides<>, NodeProxy>{
            { this, p.begin(), p.end(), PortType::In }
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
        return iterator_instantiator<iterate_by_port<>, NodeProxy>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
    }

    /**
     * @brief Proxy object that yields `NodeId_t`. Keeps track of visited
     * nodes.
     */
    struct UniqueNodeProxy
    {
        using value_type = NodeId_t const;
        using reference  = value_type&;
        using pointer    = value_type*;

        // cache to keep track of visited nodes
        QVarLengthArray<NodeId_t, 10> visited;

        template <typename Iter>
        void init(Iter& i)
        {
            if (i.s.isValid()) visited << get(i);
        }

        template <typename Iter>
        reference get(Iter& i) { return i->node; }

        template <typename Iter>
        void next(Iter& i)
        {
            while (i.s.isValid() && visited.contains(get(i))) ++i;
            if (i.s.isValid()) visited << get(i);
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
        return iterator_instantiator<iterate_one_side<>, UniqueNodeProxy>{
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
        return iterator_instantiator<iterate_both_sides<>, UniqueNodeProxy>{
            { this, p.begin(), p.end(), PortType::In }
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
        return iterator_instantiator<iterate_by_port<>, UniqueNodeProxy>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
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

    using iterator = typename data_type::iterator;
    using const_iterator = typename data_type::const_iterator;
    using key_value_iterator = typename data_type::key_value_iterator;
    using const_key_value_iterator = typename data_type::const_key_value_iterator;

    //**** custom ****//
    iterator insert(key_type const& key, Node* node)
    {
        assert(node);
        return m_data.insert(key, { node });
    }

    /**
     * @brief Convenience function. Exposes `iterate` method of the entry
     * denoted by `nodeId`. If entry was not found and empty range is returned.
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
     * entry denoted by `nodeId`. If entry was not found and empty range is
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
     * entry denoted by `nodeId`. If entry was not found and empty range is
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
     * entry denoted by `nodeId`. If entry was not found and empty range is
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

    iterator erase(iterator iter) { return m_data.erase(iter); }
    iterator erase(const_iterator iter) { return m_data.erase(iter); }

    iterator find(key_type const& key) { return m_data.find(key); }
    const_iterator find(key_type const& key) const { return m_data.find(key); }

    bool containts(key_type const& key) const { return m_data.contains(key); }

    value_type value(key_type const& key) const { return m_data.value(key); }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    key_value_iterator keyValueBegin() { return m_data.keyValueBegin(); }
    key_value_iterator keyValueEnd() { return m_data.keyValueEnd(); }
    const_key_value_iterator keyValueBegin() const { return m_data.keyValueBegin(); }
    const_key_value_iterator keyValueEnd() const { return m_data.keyValueEnd(); }

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
