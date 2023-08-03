/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphexecutorfactory.h"

#include "gt_intelligraphsequentialexecutor.h"
#include "gt_intelligraphparallelexecutor.h"

// pseudo factory
std::unique_ptr<GtIntelliGraphExecutor>
GtIntelliGraphExecutorFactory::makeExecutor(gt::ig::ExecutorType type)
{
    using namespace gt::ig;

    switch (type)
    {
    case NoExecutor:
        return {};
    case SequentialExecutor:
        return std::make_unique<GtIntelliGraphSequentialExecutor>();
    case DefaultExecutor:
    case ParallelExecutor:
        return std::make_unique<GtIntelliGraphParallelExecutor>();
    }

    gtError() << QObject::tr("Invalid Executor Type '%1'!").arg(type);
    return {};
}
