/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_PROXYITERATOR_H
#define GT_INTELLI_PROXYITERATOR_H

#include <utility>
#include <algorithm>

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
 * of iterators. Proxy objects can alter the return value of the iterator and
 * the "next" iterator.
 *
 * The proxy object must implement
 *  - `init(base_iterator)` method
 *  - `get(base_iterator)` method that returns a reference or value
 *  - `advance(base_iterator)` method that advances the base iterator to the
 *  next valid position.
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

    Iterator i{};  /// base iterator
    Proxy proxy{}; /// proxy object

    proxy_iterator() = default;
    explicit proxy_iterator(Iterator i_, Proxy p_ = {}) :
        i(std::move(i_)), proxy(std::move(p_))
    { proxy.init(i); }

    Iterator base() { return i; }

    reference operator*() { return proxy.get(i); }
    pointer operator->() { return &(operator*()); };

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
 * @brief Helper struct to instantiate a begin and end iterator for a
 * given range and installs the proxy object. Adds methods for accessing the
 * size of the range and allows reverting iterator directions if its supported
 * by proxy object.
 *
 * The proxy object can alter the return value of the iterator and the "next"
 * iterator.
 */
template <typename Iterator, typename Proxy>
struct proxy_iterable
{
    proxy_iterable(Iterator b_ = {}, Iterator e_ = {}, Proxy p_ = {}) :
        b(std::move(b_)), e(std::move(e_)), p{std::move(p_)}
    {}

    using ProxyIter = proxy_iterator<Iterator, Proxy>;

    ProxyIter begin() const { return ProxyIter{ b, p }; }
    ProxyIter end() const { return ProxyIter{ e, p }; }

    bool empty() const { return begin() == end(); }
    size_t size() const { return std::distance(begin(), end()); }

    auto reverse() const {
        using reversed_type = typename Iterator::reversed_type;
        return proxy_iterable<reversed_type, Proxy>{
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
    return proxy_iterable<Iterator, Proxy>{
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


} // namespace intelli

#endif // GT_INTELLI_PROXYITERATOR_H
