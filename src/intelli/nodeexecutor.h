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

namespace exec
{

GT_INTELLI_EXPORT bool blockingEvaluation(Node& node, NodeDataInterface& model);

GT_INTELLI_EXPORT bool detachedEvaluation(Node& node, NodeDataInterface& model);

GT_INTELLI_EXPORT void setNodeDataInterface(Node& node, NodeDataInterface& model);

GT_INTELLI_EXPORT NodeDataInterface* nodeDataInterface(Node& node);

bool triggerNodeEvaluation(Node& node, NodeDataInterface& model);

}

} // namespace intelli

#endif // GT_INTELLI_EXECUTOR_H
