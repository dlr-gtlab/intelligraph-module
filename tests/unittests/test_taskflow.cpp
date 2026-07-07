/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"
#include "node/test_dynamic.h"

#include "intelli/connection.h"
#include "intelli/utilities.h"
#include "intelli/taskflowdatamodel.h"

#include "gt_eventloop.h"

#include <taskflow/taskflow.hpp>

using namespace intelli;

TEST(TaskFlow, execute_plain_graph)
{
    Graph graph;
    test::buildBasicGraph(graph);

    auto& conModel = graph.globalConnectionModel();

    TaskFlowDataModel dataModel{graph};

    tf::Taskflow taskflow;

    QMap<NodeUuid, tf::Task> map;

    // add tasks to taskflow
    for (auto& entry : conModel)
    {
        auto task = taskflow.emplace([&entry, &dataModel](){
            gtDebug() << "evaluating" << entry.node->objectName();
            exec::blockingEvaluation(*entry.node, &dataModel);
            gtDebug() << "->" << entry.node->objectName() << "done!";
        });

        task.name(entry.node->objectName().toStdString());
        map.insert(entry.node->uuid(), task);
    }

    // define relations
    for (auto& entry : conModel)
    {
        for (auto& conDetail : entry.ports(PortType::In))
        {
            map[entry.node->uuid()].succeed(map[conDetail.node]);
        }
    }

    // execute
    tf::Executor executor;
    tf::Future<void> future = executor.run(taskflow);
    future.get();

    NodeDataSet data;
    gtDebug() << "Accessing data of node E";
    data = dataModel.nodeData(E_uuid, PortType::In, PortIndex{0});
    EXPECT_EQ(data.as<DoubleData>()->value(), 8.0);
    gtDebug() << "Accessing data of node D";
    data = dataModel.nodeData(D_uuid, PortType::Out, PortIndex{0});
    EXPECT_EQ(data.as<DoubleData>()->value(), 42.0);
}

TEST(TaskFlow, execute_plain_graph_async)
{
    Graph graph;
    test::buildBasicGraph(graph);

    auto& conModel = graph.globalConnectionModel();

    TaskFlowDataModel dataModel{graph};

    tf::Taskflow taskflow;

    QMap<NodeUuid, tf::Task> map;

    // add tasks to taskflow
    for (auto& entry : conModel)
    {
        auto task = taskflow.emplace([&entry, &dataModel](){
            gtDebug() << "evaluating" << entry.node->objectName();

            GtEventLoop eventLoop{200};

            eventLoop.connectSuccess(entry.node.data(), &Node::evaluated);
            exec::detachedEvaluation(*entry.node, &dataModel);

            GtEventLoop::State status = eventLoop.exec();
            gtDebug() << "->" << entry.node->objectName() << "done!" << status;
        });

        task.name(entry.node->objectName().toStdString());
        map.insert(entry.node->uuid(), task);
    }

    // define relations
    for (auto& entry : conModel)
    {
        for (auto& conDetail : entry.ports(PortType::In))
        {
            map[entry.node->uuid()].succeed(map[conDetail.node]);
        }
    }

    // execute
    tf::Executor executor;
    tf::Future<void> future = executor.run(taskflow);
    future.get();

    NodeDataSet data;
    gtDebug() << "Accessing data of node E";
    data = dataModel.nodeData(E_uuid, PortType::In, PortIndex{0});
    EXPECT_EQ(data.as<DoubleData>()->value(), 8.0);
    gtDebug() << "Accessing data of node D";
    data = dataModel.nodeData(D_uuid, PortType::Out, PortIndex{0});
    EXPECT_EQ(data.as<DoubleData>()->value(), 42.0);
}
