/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHPARALLELEXECUTOR_H
#define GTINTELLIGRAPHPARALLELEXECUTOR_H

#include "gt_intelligraphexecutor.h"
#include "gt_intelligraphnode.h"

#include <QFutureWatcher>

class GtIntelliGraphParallelExecutor : public GtIntellIGraphExecutor
{
    Q_OBJECT

public:

    using NodeData = GtIntelliGraphNode::NodeData;

    GtIntelliGraphParallelExecutor();
    ~GtIntelliGraphParallelExecutor();

    bool evaluateNode(GtIntelliGraphNode& node) override;

    bool evaluatePort(GtIntelliGraphNode& node, PortIndex idx) override;

    bool isReady() const override;

protected:

    bool canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx = PortIndex{}) override;

private:

    QPointer<GtIntelliGraphNode> m_node;

    QFutureWatcher<std::vector<NodeData>> m_watcher;

    bool m_collected = true;

private slots:

    void onStarted();
    void onFinished();
    void onCanceled();
    void onResultReady(int idx);
    void onResultsReady(int begin, int end);
};

#endif // GTINTELLIGRAPHPARALLELEXECUTOR_H
