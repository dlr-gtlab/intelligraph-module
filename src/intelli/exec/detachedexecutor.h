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

#include <intelli/globals.h>

#include <QFutureWatcher>
#include <QPointer>

namespace intelli
{

class Node;
class NodeDataInterface;

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

    bool evaluateNode(Node& node, NodeDataInterface& model);

    bool canEvaluateNode();

private:
    
    QPointer<Node> m_node;
    
    struct ReturnValue
    {
        NodeDataPtrList data;
        bool success = false;
    };

    QFutureWatcher<ReturnValue> m_watcher;

    bool m_collected = true;
    bool m_destroyed = false;

private slots:

    void onFinished();
    void onCanceled();
    void onResultReady(int idx);
};

} // namespace intelli

#endif // GT_INTELLI_DETACHEDEXECUTOR_H
