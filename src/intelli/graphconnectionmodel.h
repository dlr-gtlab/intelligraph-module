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

template <typename Container, typename T>
struct begin_iterator
{
    auto operator()(Container const& c) const { return c.begin(); }
};
template <typename Container>
struct begin_iterator<Container, typename Container::const_reverse_iterator>
{
    auto operator()(Container const& c) const { return c.rbegin(); }
};

template <typename Container, typename T>
struct end_iterator
{
    auto operator()(Container const& c) const { return c.end(); }
};
template <typename Container>
struct end_iterator<Container, typename Container::const_reverse_iterator>
{
    auto operator()(Container const& c) const { return c.rend(); }
};

} // namespace dtail;

/// directed acyclic graph representing connections and nodes
namespace connection_model
{

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
     * @brief Creates an outgoing connection id.
     * @param sourceNode Source (outgoing) node
     * @return Connection id
     */
    ConnectionId_t<NodeId_t> toConnection(NodeId_t sourceNode) const
    {
        return { std::move(sourceNode), sourcePort, node, port };
    }
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
        Strategy s{}; /// iteration strategy

        using iterator_category = std::forward_iterator_tag;
        using value_type = typename Strategy::value_type;
        using difference_type = ptrdiff_t;
        using pointer = value_type;
        using reference = value_type;

        base_iterator() = default;
        explicit base_iterator(Strategy data_) : s(std::move(data_)) { }

        value_type operator*() const { return s.get(); }
        value_type operator->() const { return *this; }

        bool operator==(base_iterator const& o) const { return s == o.s; }
        bool operator!=(base_iterator const& o) const { return !(operator==(o)); }

        /// pre increment
        base_iterator& operator++()
        {
            s.inc();
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
    template <typename Strategy>
    struct iterator_instantiator
    {
        iterator_instantiator(Strategy b_, Strategy e_ = {}) :
            b(std::move(b_)), e(std::move(e_))
        {}

        base_iterator<Strategy> begin() const { return base_iterator<Strategy>{b}; }
        base_iterator<Strategy> end() const { return base_iterator<Strategy>{e}; }
        bool empty() const { return begin() == end(); }

        /**
         * @brief Reverses the iterator
         */
        auto reverse() {
            // chose reverse or forward iter based on condition
            using Cond = std::is_same<decltype(b.iter), const_forward_iter>;
            using iter = std::conditional_t<
                Cond::value, const_reverse_iter, const_forward_iter>;
            // build new type
            using S = decltype(b.template reverse<iter>());
            return iterator_instantiator<S>{
                b.template reverse<iter>(),
                e.template reverse<iter>()
            };
        }

    private:
        Strategy b, e;
    };

    //**** iterator strategies ****//

    /**
     * @brief Strategy to iterate only over all ingoing OR outgoing connections
     * of this object
     */
    template <typename container_iterator>
    struct iterate_connections_one_side
    {
        using value_type = ConnectionId_t<NodeId_t>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator const end{};
        PortType const type{PortType::NoType};

        iterate_connections_one_side() { assert(!isValid()); }
        iterate_connections_one_side(ConnectionData const* ptr_,
                                     container_iterator begin_,
                                     container_iterator end_,
                                     PortType type_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_)
        { }

        template <typename iter,
                  typename return_type = iterate_connections_one_side<iter>>
        return_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<container_type, iter>{}(p),
                detail::end_iterator<container_type, iter>{}(p),
                type
            };
        }

        bool isValid() const {
            return ptr && type != PortType::NoType && iter != end;
        }

        value_type get() const {
            assert(isValid() && "accessed invalid iterator! (iterate_connections_oneside)");
            return iter->toConnection(get_node_id<NodeId_t>{}(ptr->node), type);
        }

        bool operator==(iterate_connections_one_side const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type) ;
        }

        void inc()
        {
            if (iter != end) ++iter;
        }
    };

    /**
     * @brief Strategy to iterate over all ingoing AND outgoing connections
     * of this object
     */
    template <typename container_iterator>
    struct iterate_connections_both_sides
    {
        using value_type = ConnectionId_t<NodeId_t>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator end{};
        PortType type{PortType::NoType};

        iterate_connections_both_sides() { assert(!isValid()); }
        iterate_connections_both_sides(ConnectionData const* ptr_,
                                       container_iterator begin_,
                                       container_iterator end_,
                                       PortType type_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_)
        {
            if (iter == end) inc();
        }

        template <typename iter,
                  typename return_type = iterate_connections_both_sides<iter>>
        return_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<container_type, iter>{}(p),
                detail::end_iterator<container_type, iter>{}(p),
                type
            };
        }

        bool isValid() const
        {
            return ptr && type != PortType::NoType && iter != end;
        }

        value_type get() const
        {
            assert(isValid() && "accessed invalid iterator! (iterate_connections)");
            return iter->toConnection(get_node_id<NodeId_t>{}(ptr->node), type);
        }

        bool operator==(iterate_connections_both_sides const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type);
        }

        void inc()
        {
            if (iter != end) ++iter;
            // finsihed iterating input side -> switch to output side
            if (iter == end && type == PortType::In)
            {
                auto& p = ptr->ports(PortType::Out);
                iter = detail::begin_iterator<container_type, container_iterator>{}(p);
                end  = detail::end_iterator<container_type, container_iterator>{}(p);
                type = PortType::Out;
            }
        }
    };

    /**
     * @brief Strategy to iterate over all connections that either end or start
     * at a specific Port (denoted by a port id)
     */
    template <typename container_iterator>
    struct iterate_connections_by_port
    {
        using value_type = ConnectionId_t<NodeId_t>;

        ConnectionData const* ptr{};
        container_iterator iter{};
        container_iterator end{};
        PortType type{PortType::NoType};
        PortId const portId{};

        iterate_connections_by_port() { assert(!isValid()); }
        iterate_connections_by_port(ConnectionData const* ptr_,
                                    container_iterator begin_,
                                    container_iterator end_,
                                    PortType type_,
                                    PortId portId_) :
            ptr(ptr_), iter(begin_), end(end_), type(type_), portId(portId_)
        {
            if (iter == end || iter->sourcePort != portId) inc();
        }

        template <typename iter,
                 typename return_type = iterate_connections_by_port<iter>>
        return_type reverse() const
        {
            if (!ptr) return {};
            auto& p = ptr->ports(type);
            return {
                ptr,
                detail::begin_iterator<container_type, iter>{}(p),
                detail::end_iterator<container_type, iter>{}(p),
                type,
                portId
            };
        }

        bool isValid() const
        {
            return ptr && type != PortType::NoType && iter != end;
        }

        value_type get() const
        {
            assert(isValid() && "accessed invalid iterator! (iterate_connections_by_port)");
            return iter->toConnection(get_node_id<NodeId_t>{}(ptr->node), type);
        }

        bool operator==(iterate_connections_by_port const& o) const
        {
            return (!isValid() && !o.ptr) || (!ptr && !o.isValid()) ||
                   (ptr == o.ptr && iter == o.iter && type == o.type && portId == o.portId);
        }

        void inc()
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
                    iter = detail::begin_iterator<container_type, container_iterator>{}(p);
                    end  = detail::end_iterator<container_type, container_iterator>{}(p);
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
     * @brief Strategy to iterate only over all connected nodes at the inside OR
     * outside
     */
    template <typename container_iterator>
    struct iterate_nodes_one_side : public iterate_connections_one_side<container_iterator>
    {
        using base_class = iterate_connections_one_side<container_iterator>;
        using value_type = NodeId_t const&;

        using base_class::base_class;

        value_type get() const
        {
            assert(base_class::isValid() && "accessed invalid iterator! (iterate_nodes_one_side)");
            return base_class::iter->node;
        }

        template <typename iter>
        iterate_nodes_one_side<iter> reverse() const
        {
            return base_class::template reverse<iter, iterate_nodes_one_side<iter>>();
        }
    };

    /**
     * @brief Strategy to iterate over all connected nodes at the inside
     * and outside
     */
    template <typename container_iterator>
    struct iterate_nodes_both_sides : public iterate_connections_both_sides<container_iterator>
    {
        using base_class = iterate_connections_both_sides<container_iterator>;
        using value_type = NodeId_t const&;

        using base_class::base_class;

        value_type get() const
        {
            assert(base_class::isValid() && "accessed invalid iterator! (iterate_nodes_both_sides)");
            return base_class::iter->node;
        }

        template <typename iter>
        iterate_nodes_both_sides<iter> reverse() const
        {
            return base_class::template reverse<iter, iterate_nodes_both_sides<iter>>();
        }
    };

    /**
     * @brief Strategy to iterate over all nodes that are connected to a
     * specific Port (denoted by a port id)
     */
    template <typename container_iterator>
    struct iterate_nodes_by_port : public iterate_connections_by_port<container_iterator>
    {
        using base_class = iterate_connections_by_port<container_iterator>;
        using value_type = NodeId_t const&;

        using base_class::base_class;

        value_type get() const
        {
            assert(base_class::isValid() && "accessed invalid iterator! (iterate_nodes_by_port)");
            return base_class::iter->node;
        }

        template <typename iter>
        iterate_nodes_by_port<iter> reverse() const
        {
            return base_class::template reverse<iter, iterate_nodes_by_port<iter>>();
        }
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
        return iterator_instantiator<iterate_connections_one_side<const_forward_iter>>{
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
        return iterator_instantiator<iterate_connections_both_sides<const_forward_iter>>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }

    /**
     * @brief Can be used to iterate over all connections that either end or
     * start at the given port id. Will iterate over all input and output ports
     * internally.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnections(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<iterate_connections_by_port<const_forward_iter>>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
    }

    /**
     * @brief Can be used to iterate over all nodes connected to either the
     * inside or outside depending on the given port type.
     * @param type Port type, denoting whether to iterate over ingoing or
     * outgoing connections
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnectedNodes(PortType type) const
    {
        auto& p = ports(type);
        return iterator_instantiator<iterate_nodes_one_side<const_forward_iter>>{
            { this, p.begin(), p.end(), type }
        };
    }

    /**
     * @brief Can be used to iterate over all connetced nodes. Will iterate
     * over all input and output ports internally.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnectedNodes() const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<iterate_nodes_both_sides<const_forward_iter>>{
            { this, p.begin(), p.end(), PortType::In }
        };
    }

    /**
     * @brief Can be used to iterate over all nodes that are connected to the
     * given port id. Will iterate over all input and output ports internally.
     * @return Helper object to call begin and end on. Can be used easily
     * with range-based for loops
     */
    auto iterateConnectedNodes(PortId portId) const
    {
        auto& p = ports(PortType::In);
        return iterator_instantiator<iterate_nodes_by_port<const_forward_iter>>{
            { this, p.begin(), p.end(), PortType::In, portId }
        };
    }
};

template <typename T>
class ConnectionModel
{
public:

    using key_type = T;
    using value_type = ConnectionData<T>;
    using data_type = QHash<key_type, value_type>;

    using iterator = typename data_type::iterator;
    using const_iterator = typename data_type::const_iterator;
    using key_value_iterator = typename data_type::key_value_iterator;
    using const_key_value_iterator = typename data_type::const_key_value_iterator;

    //**** custom ****//
    iterator insert(key_type const& key, Node* node) { assert(node); return m_data.insert(key, { node }); }

    //**** QHash ****//
    iterator insert(key_type const& key, value_type const& value) { return m_data.insert(key, value); }
    void insert(ConnectionModel const& other) { m_data.insert(other.m_data); }

    iterator erase(iterator iter) { return m_data.erase(iter); }
    iterator erase(const_iterator iter) { return m_data.erase(iter); }

    iterator find(key_type const& key) { return m_data.find(key); }
    const_iterator find(key_type const& key) const { return m_data.find(key); }

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

template <typename Model, typename NodeId_t>
inline auto*
find(Model& model, NodeId_t nodeId)
{
    using T = decltype(&(*model.begin()));

    auto iter = model.find(nodeId);
    if (iter == model.end()) return T(nullptr);
    return &(*iter);
}

struct TraverseRecursively{};

template <typename ConnectionData_t, typename Lambda>
inline void
visit(ConnectionData_t& data, PortId sourcePort, PortType type, Lambda const& lambda)
{
    return visit(data, type, [sourcePort, type, &lambda](auto& con){
        if (con.sourcePort == sourcePort)
        {
            lambda(con);
        }
    });
}

template <typename ConnectionData_t, typename Lambda>
inline void
visit(ConnectionData_t& data, PortType type, Lambda const& lambda)
{
    static_assert(!std::is_pointer<ConnectionData_t>::value,
                  "expected ConnectionData_t& but got ConnectionData_t*");
    for (auto& con : data.ports(type))
    {
        lambda(con);
    }
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitSuccessors(ConnectionData_t& data, PortId sourcePort, Lambda const& lambda)
{
    return visit(data, sourcePort, PortType::Out, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitSuccessors(ConnectionData_t& data, Lambda const& lambda)
{
    return visit(data, PortType::Out, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitPredecessors(ConnectionData_t& data, PortId sourcePort, Lambda const& lambda)
{
    return visit(data, sourcePort, PortType::In, lambda);
}

template <typename ConnectionData_t, typename Lambda>
inline void
visitPredecessors(ConnectionData_t& data, Lambda const& lambda)
{
    return visit(data, PortType::In, lambda);
}

template <typename ConnectionData_t>
inline bool
hasConnections(ConnectionData_t& data, PortId sourcePort, PortType type)
{
    size_t count = 0;
    visit(data, sourcePort, type, [&count](auto&){ ++count; return true; });
    return count;
}

template <typename ConnectionData_t>
inline bool
hasConnections(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::In) ||
           hasConnections(data, sourcePort, PortType::Out);
}

template <typename ConnectionData_t>
inline bool
hasSuccessors(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::Out);
}

template <typename ConnectionData_t>
inline bool
hasPredecessors(ConnectionData_t& data, PortId sourcePort)
{
    return hasConnections(data, sourcePort, PortType::In);
}

template <typename ConnectionData_t>
inline bool
hasSuccessors(ConnectionData_t& data)
{
    return !data->successors.empty();
}

template <typename ConnectionData_t>
inline bool
hasPredecessors(ConnectionData_t& data)
{
    return !data->predecessors.empty();
}

template <typename ConnectionData_t, typename NodeId_t>
inline bool
containsConnection(ConnectionData_t& data,
                   NodeId_t const& sourceNode,
                   ConnectionId_t<NodeId_t> const& conId,
                   PortType type)
{
    static_assert(!std::is_pointer<ConnectionData_t>::value,
                  "expected ConnectionData_t& but got ConnectionData_t*");
    for (auto& con : data.ports(type))
    {
        if (con.toConnection(sourceNode, type) == conId) return true;
    }
    return false;
}

// operators
template <typename T>
inline bool operator==(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b)
{
    return a.node == b.node && a.port == b.port && a.sourcePort == b.sourcePort;
}

template <typename T>
inline bool operator!=(ConnectionDetail<T> const& a, ConnectionDetail<T> const& b) { return !(a == b); }

// definitions
using GlobalConnectionModel = ConnectionModel<NodeUuid>;
using LocalConnectionModel  = ConnectionModel<NodeId>;

} // namespace connection_model

using connection_model::ConnectionModel;
using connection_model::LocalConnectionModel;
using connection_model::GlobalConnectionModel;

} // namespace intelli;

#endif // GT_INTELLI_GRAPHCONNECTIONMODEL_H
