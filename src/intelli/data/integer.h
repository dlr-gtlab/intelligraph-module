/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_INTEGERDATA_H
#define GT_INTELLI_INTEGERDATA_H

#include <intelli/data/int.h>

namespace intelli
{

using IntegerData[[deprecated("Use `IntData` instead (note: include `intelli/data/int.h` instead)")]] = IntData;

} // namespace intelli

#endif // GT_INTELLI_INTEGERDATA_H
