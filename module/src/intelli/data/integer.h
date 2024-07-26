/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  E-Mail: jens.schmeink@dlr.de
 */

#ifndef GT_INTELLI_INTEGERDATA_H
#define GT_INTELLI_INTEGERDATA_H

#include "intelli/data/int.h"

namespace intelli
{

using IntegerData[[deprecated("Use `IntData` instead (note: include `intelli/data/int.h` instead)")]] = IntData;

} // namespace intelli

#endif // GT_INTELLI_INTEGERDATA_H