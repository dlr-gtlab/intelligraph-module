/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SEQUENTIALEXECUTOR_H
#define GT_INTELLI_SEQUENTIALEXECUTOR_H

#include "intelli/exec/executor.h"

namespace intelli
{

class SequentialExecutor : public Executor
{
    Q_OBJECT

public:
    
    bool evaluateNode(Node& node) override;
    
    bool evaluatePort(Node& node, PortIndex idx) override;

protected:
    
    bool canEvaluateNode(Node& node, PortIndex outIdx = PortIndex{}) override;

private:

    bool m_evaluating = false;
};

} // namespace intelli

#endif // GT_INTELLI_SEQUENTIALEXECUTOR_H
