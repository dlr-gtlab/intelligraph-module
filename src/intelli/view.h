/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_VIEW_H
#define GT_INTELLI_VIEW_H

#include <intelli/span.h>

namespace intelli
{

/** -- ADAPTED FROM NUMBAS LIBRARY --
 * @brief Wrapper class to view contiguous memory containers.
 * Can be used to "view" (iterate over, access but NOT modify data of a
 * vector-like type.
 *
 * Note: Does not work with temporary containers!
 *
 * Usage:
 *
 * int myFunction(View<int> myInts) {
 *     if (myInts.size() > 0) {
 *         return myInts[0];
 *     }
 *     return 42;
 * }
 *
 * {
 *     std::vector<int> ints = {0, 1, 2, 3};
 *     int res = myFunction(ints);
 * }
 */
template <typename T>
class View : public Span<T const>
{
    using base_class = Span<T const>;
public:

    using value_type      = typename base_class::value_type;
    using size_type       = typename base_class::size_type;

    using base_class::operator[];

    View(T const* data, size_type size) :
        base_class(data, size)
    {}

    View(std::initializer_list<T> list) :
        base_class(list.begin(), std::distance(list.begin(), list.end()))
    {}

    template<template<class...> class Container,
             typename... U,
             std::enable_if_t<
                 std::is_convertible<typename Container<U...>::value_type,
                                     T const>::value, bool> = true>
    View(Container<U...> const& vector) :
        base_class(detail::container_data<T const>::get(vector),
                   detail::container_size<size_type>::get(vector))
    {}

    template <size_t N>
    View(std::array<T, N> const& array) : View(array.data(), N) {}

    /// disabled for r-values
    template<template<class...> class Container, typename... U>
    View(Container<U...>&& vector) = delete;
};

} // namespace intelli

#endif // GT_INTELLI_VIEW_H
