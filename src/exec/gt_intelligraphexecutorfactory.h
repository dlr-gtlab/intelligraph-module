/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHEXECUTORFACTORY_H
#define GTINTELLIGRAPHEXECUTORFACTORY_H

#include "gt_intelligraph_exports.h"
#include "gt_igglobals.h"

#include <memory>

class GtIntelliGraphExecutor;
class  GtIntelliGraphExecutorFactory
{
public:

    using Executor = std::unique_ptr<GtIntelliGraphExecutor>;
    GT_IG_EXPORT static Executor makeExecutor(gt::ig::ExecutorType type);
};

#endif // GTINTELLIGRAPHEXECUTORFACTORY_H
