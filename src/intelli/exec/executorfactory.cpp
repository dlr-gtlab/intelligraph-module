/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/executorfactory.h"

#include "intelli/exec/executor.h"
//#include "intelli/exec/sequentialexecutor.h"
//#include "intelli/exec/parallelexecutor.h"

using namespace intelli;

// pseudo factory
std::unique_ptr<intelli::Executor>
ExecutorFactory::makeExecutor(ExecutionMode type)
{
    using namespace intelli;

    switch (type)
    {
    case ExecutionMode::None:
        return {};
    case ExecutionMode::Sequential:
//        return std::make_unique<SequentialExecutor>();
    case ExecutionMode::Default:
    case ExecutionMode::Parallel:
//        return std::make_unique<ParallelExecutor>();
        break;
    }

    gtError() << QObject::tr("Invalid Executor Type '%1'!").arg(static_cast<int>(type));
    return {};
}
