/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/executorfactory.h"

#include "intelli/exec/sequentialexecutor.h"
#include "intelli/exec/parallelexecutor.h"

using namespace intelli;

// pseudo factory
std::unique_ptr<intelli::Executor>
ExecutorFactory::makeExecutor(ExecutorMode type)
{
    using namespace intelli;

    switch (type)
    {
    case ExecutorMode::None:
        return {};
    case ExecutorMode::Sequential:
        return std::make_unique<SequentialExecutor>();
    case ExecutorMode::Default:
    case ExecutorMode::Parallel:
        return std::make_unique<ParallelExecutor>();
    }

    gtError() << QObject::tr("Invalid Executor Type '%1'!").arg(static_cast<int>(type));
    return {};
}
