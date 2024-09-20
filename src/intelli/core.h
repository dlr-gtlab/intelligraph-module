/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marting Siggel <martin.siggel@dlr.de>
 */

#ifndef INTELLI_CORE_H
#define INTELLI_CORE_H

#include <intelli/exports.h>

namespace intelli
{

GT_INTELLI_EXPORT void initModule();

GT_INTELLI_EXPORT void registerDefaultDataTypes();

GT_INTELLI_EXPORT void registerDefaultNodes();

} // namespace intelli

#endif // INTELLI_CORE_H
