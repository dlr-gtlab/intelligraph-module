/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_SPAN_H
#define GT_INTELLI_SPAN_H

#include <type_traits>
#include <array>

namespace intelli
{

namespace detail
{

/// helper struct for accessing data pointer of Container
/// -- ADAPTED FROM NUMBAS LIBRARY -- ///
template <typename T>
struct container_data
{
    template <typename C,
             std::enable_if_t<
                 std::is_convertible<decltype(std::declval<C&>().data()),
                                     T*>::value, bool> = true>
    static T* get(C& c) { return c.data(); }
};

/// helper struct for accessing size property of Container
/// -- ADAPTED FROM NUMBAS LIBRARY -- ///
template <typename size_type>
struct container_size
{
    template <typename C,
             std::enable_if_t<
                 std::is_convertible<decltype(std::declval<C&>().size()),
                                     size_type>::value, bool> = true>
    static size_type get(C& c) { return c.size(); }
};

} // namespace detail

/** -- ADAPTED FROM NUMBAS LIBRARY --
 * @brief Wrapper class to access contiguous memory containers.
 * Can be used to iterate over, access and modify data of a vector-like type
 * (no append or remove operations supported).
 *
 * Note: Does not work with temporary containers!
 *
 * Usage:
 *
 * void myFunction(Span<int> myInts) {
 *     if (myInts.size() > 0) {
 *         myInts[0] = 42;
 *     }
 * }
 *
 * {
 *     std::vector<int> ints = {0, 1, 2, 3};
 *     myFunction(ints);
 * }
 */
template <typename T>
class Span
{
public:

    using value_type      = T;
    using size_type       = size_t;
    using iterator        = T*;
    using const_iterator  = T const*;

    using pointer         = T*;
    using const_pointer   = T const*;
    using reference       = T&;
    using const_reference = T const&;

    /**
     * @brief Span
     * @param data Data pointer
     * @param size Size of pointer. Size must not exceed actual size of data
     */
    Span(T* data, size_type size) :
        m_data(data),
        m_size(size)
    {}

    template<template<class...> class Container,
             typename... U,
             std::enable_if_t<
                std::is_convertible<typename Container<U...>::value_type,
                                    T>::value, bool> = true>
    Span(Container<U...>& vector) :
        Span(detail::container_data<T>::get(vector),
             detail::container_size<size_type>::get(vector))
    {}

    template <size_t N>
    Span(std::array<T, N>& array) : Span(array.data(), N) {}

    /// disabled for const l- and r-values
    template<template<class...> class Container, typename... U>
    Span(Container<U...> const& vector) = delete;

    /**
     * @brief Whether the span is valid
     * @return is null
     */
    bool null() const { return m_data == nullptr; }

    /**
     * @brief Whether the span is empty
     * @return is empty
     */
    bool empty() const { return size() == 0; }

    /**
     * @brief size
     * @return size of span
     */
    size_type size() const { return m_size; }

    /* index operators */
    reference operator[](size_type i) { return m_data[i]; }
    const_reference operator[](size_type i) const { return m_data[i]; }

    /* at */
    reference at(size_type idx)
    {
        assert(idx < size());
        return operator[](idx);
    }
    const_reference at(size_type idx) const
    {
        assert(idx < size());
        return operator[](idx);
    }

    /* begin-iterators */
    iterator begin() { return m_data; }
    const_iterator begin() const noexcept { return m_data; }
    const_iterator cbegin() const noexcept { return begin(); }

    /* end-iterators */
    iterator end() { return m_data + m_size; }
    const_iterator end() const noexcept { return m_data + m_size; }
    const_iterator cend() const noexcept { return end(); }

    /*  getter for data ptr */
    value_type* data() { return m_data; }
    value_type const* data() const { return m_data; }
    value_type const* constData() const { return data(); }

    /**
     * @brief front
     * @return front
     */
    reference front() { return at(0); }
    const_reference front() const { return at(0); }

    /**
     * @brief back
     * @return back
     */
    reference back() { return m_data[m_size-1]; }
    const_reference back() const { return m_data[m_size-1]; }

private:

    T* m_data;
    size_type const m_size;
};

} // namespace intelli

#endif // GT_INTELLI_SPAN_H
