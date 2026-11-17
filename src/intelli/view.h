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
using View = Span<T const>;

} // namespace intelli

#endif // GT_INTELLI_VIEW_H
