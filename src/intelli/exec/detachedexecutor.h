/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DETACHEDEXECUTOR_H
#define GT_INTELLI_DETACHEDEXECUTOR_H

#include "intelli/nodeexecutor.h"
#include "intelli/node.h"

#include <QFutureWatcher>
#include <QPointer>

namespace intelli
{

/**
 * @brief The DetachedExecutor class.
 * Executes a node in separate thread to allow parallelism
 */
class DetachedExecutor : public QObject
{
    Q_OBJECT

public:
    
    DetachedExecutor();
    ~DetachedExecutor();

    bool evaluateNode(Node& node, GraphExecutionModel& model);

protected:
    
    bool canEvaluateNode(Node& node);

private:
    
    QPointer<Node> m_node;
    
    QFutureWatcher<NodeDataPtrList> m_watcher;

    bool m_collected = true;

private slots:

    void onFinished();
    void onCanceled();
    void onResultReady(int idx);
};

} // namespace intelli

#endif // GT_INTELLI_DETACHEDEXECUTOR_H
