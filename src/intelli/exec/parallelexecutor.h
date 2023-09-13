/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_PARALLELEXECUTOR_H
#define GT_INTELLI_PARALLELEXECUTOR_H

#include "intelli/exec/executor.h"
#include "intelli/node.h"

#include <QFutureWatcher>
#include <QPointer>

namespace intelli
{

class ParallelExecutor : public Executor
{
    Q_OBJECT

public:
    
    using NodeDataPtr = Node::NodeDataPtr;

    ParallelExecutor();
    ~ParallelExecutor();

    bool evaluateNode(Node& node, GraphExecutionModel& model, PortId portId = invalid<PortId>()) override;

    bool isReady() const override;

protected:
    
    bool canEvaluateNode(Node& node);

private:
    
    QPointer<Node> m_node;
    
    QFutureWatcher<NodeDataPtrList> m_watcher;

    PortId m_port;

    bool m_collected = true;

private slots:

    void onFinished();
    void onCanceled();
    void onResultReady(int idx);
};

} // namespace intelli

#endif // GT_INTELLI_PARALLELEXECUTOR_H
