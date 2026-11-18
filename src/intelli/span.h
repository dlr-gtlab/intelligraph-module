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

#include <intelli/utilities.h>

#include <type_traits>
#include <iterator>
#include <cassert>
#include <stddef.h>

namespace intelli
{

/** -- ADAPTED FROM NUMBAS LIBRARY --
 * @brief Wrapper class to access contiguous memory containers.
 * Can be used to iterate over, access and modify data of a vector-like type
 * (no append or remove operations supported).
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
    /// helper struct to check if a Container type `C` is compatible with `T`
    template <typename C, typename = void>
    struct is_compatible : std::false_type {};
    template <typename C>
    struct is_compatible<C, std::void_t<
                                // checks if std::data and std::size is supported
                                decltype(std::data(std::declval<C&>())),
                                decltype(std::size(std::declval<C&>()))>
                         > : std::is_convertible<
                                 // checks U*[] vs T*[] (array types)
                                 std::remove_pointer_t<
                                     decltype(std::data(std::declval<C&>()))>(*)[],
                                 T(*)[]>{};

    /// bypasses Qt's detach if C is not const
    template <typename C>
    static constexpr T* data_of(C& c) noexcept
    {
        if constexpr (std::is_const<T>::value) return std::data(asConst(c));
        else return std::data(c);
    }

    /// returns the underlying type
    template <typename U>
    using underlying_type_t = std::remove_cv_t<std::remove_reference_t<U>>;

public:

    using value_type      = T;
    using size_type       = size_t;
    using iterator        = T*;
    using const_iterator  = T const*;

    using pointer         = T*;
    using const_pointer   = T const*;
    using reference       = T&;
    using const_reference = T const&;

    /// default constructor
    Span() noexcept {}

    Span(Span const& other) noexcept = default;
    Span(Span&& other) noexcept = default;
    Span& operator=(Span const& other) noexcept = default;
    Span& operator=(Span&& other) noexcept = default;
    ~Span() noexcept = default;

    /**
     * @brief Span
     * @param data Data pointer
     * @param size Size of pointer. Size must not exceed actual size of data
     */
    Span(T* data, size_type size) noexcept :
        m_data(data),
        m_size(size)
    {}

    /// std::initializer_list
    template <typename U = T, typename = std::enable_if_t<std::is_const<U>::value>>
    Span(std::initializer_list<std::remove_const_t<U>> ilist) noexcept :
        Span(ilist.begin(), ilist.size())
    {}

    /// any contiguous container
    template<typename Container,
             typename = std::enable_if_t<
                 // avoid matching own constructors
                 !std::is_same<underlying_type_t<Container>, Span>::value &&
                 // must support std::data and std::size and array types must be convertible
                 is_compatible<Container>::value &&
                 // disallow r-values if T is not const
                 (std::is_const<T>::value || std::is_lvalue_reference<Container>::value)>>
    Span(Container&& vector) noexcept :
        Span(data_of(vector), std::size(vector))
    {}

    /**
     * @brief Returns whether the span is invalid
     * @return Whether the span is null
     */
    bool null() const noexcept { return m_data == nullptr; }

    /**
     * @brief Whether the span is empty
     * @return is empty
     */
    bool empty() const noexcept { return size() == 0; }

    /**
     * @brief size
     * @return size of span
     */
    size_type size() const noexcept { return m_size; }

    /* index operators */
    reference operator[](size_type i) noexcept { return m_data[i]; }
    const_reference operator[](size_type i) const noexcept { return m_data[i]; }

    /* at */
    reference at(size_type idx) noexcept
    {
        assert(!null());
        assert(idx < size());
        return operator[](idx);
    }
    const_reference at(size_type idx) const noexcept
    {
        assert(!null());
        assert(idx < size());
        return operator[](idx);
    }

    /* begin-iterators */
    iterator begin() noexcept { return m_data; }
    const_iterator begin() const noexcept { return m_data; }
    const_iterator cbegin() const noexcept { return begin(); }

    /* end-iterators */
    iterator end() noexcept { return m_data + m_size; }
    const_iterator end() const noexcept { return m_data + m_size; }
    const_iterator cend() const noexcept { return end(); }

    /*  getter for data ptr */
    value_type* data() noexcept { return m_data; }
    value_type const* data() const noexcept { return m_data; }
    value_type const* constData() const noexcept { return data(); }

    /**
     * @brief front
     * @return front
     */
    reference front() noexcept { return at(0); }
    const_reference front() const noexcept { return at(0); }

    /**
     * @brief back
     * @return back
     */
    reference back() { return at(size() - 1); }
    const_reference back() const { return at(size() - 1); }

private:

    T* m_data{};
    size_type m_size{};
};

} // namespace intelli

#endif // GT_INTELLI_SPAN_H
