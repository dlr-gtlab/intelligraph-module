/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_EXECUTOR_H
#define GT_INTELLI_EXECUTOR_H

#include "intelli/exports.h"

#include <QPointer>

namespace intelli
{

class Node;
class NodeDataInterface;
class GraphExecutionModel;

GT_INTELLI_EXPORT bool blockingEvaluation(Node& node,
                                          GraphExecutionModel& model);

GT_INTELLI_EXPORT bool detachedEvaluation(Node& node,
                                          GraphExecutionModel& model);


/**
 * @brief The NodeExecutor class. Helper class to access private or protected
 * members of a Node used for the evaluation.
 */
class NodeExecutor
{
    NodeExecutor() = delete;

public:

    static void evaluate(Node& node);

    static GraphExecutionModel* accessExecModel(Node& node);

    static void setNodeDataInterface(Node& node, NodeDataInterface* interface);
};

} // namespace intelli

#endif // GT_INTELLI_EXECUTOR_H
