/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_EXECUTORFACTORY_H
#define GT_INTELLI_EXECUTORFACTORY_H

#include "intelli/exports.h"
#include "intelli/globals.h"

#include <memory>

namespace intelli
{

class Executor;
class ExecutorFactory
{
public:

    GT_INTELLI_EXPORT static std::unique_ptr<intelli::Executor> makeExecutor(ExecutorMode type);
};

} // namespace intelli

#endif // GT_INTELLI_EXECUTORFACTORY_H
