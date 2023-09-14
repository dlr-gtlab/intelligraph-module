/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SEQUENTIALEXECUTOR_H
#define GT_INTELLI_SEQUENTIALEXECUTOR_H

#include "intelli/nodeexecutor.h"

namespace intelli
{

class BlockingExecutor : public NodeExecutor
{
    Q_OBJECT

public:

    BlockingExecutor() = default;

    bool evaluateNode(Node& node,
                      GraphExecutionModel& model,
                      PortId portId = invalid<PortId>());

};

} // namespace intelli

#endif // GT_INTELLI_SEQUENTIALEXECUTOR_H
