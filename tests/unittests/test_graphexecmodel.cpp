/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"
#include "node/test_node.h"

#include "intelli/memory.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>
#include <gt_eventloop.h>

#include <gt_algorithms.h>

using namespace intelli;

constexpr auto maxTimeout = std::chrono::seconds(1);
constexpr auto directTimeout = std::chrono::seconds(0);

/// Evaluate a single node without any dependencies
TEST(GraphExecutionModel, evaluate_node_without_dependencies)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    Node* A = graph.findNode(A_id);
    ASSERT_TRUE(A);

    /// make sure node evaluation is emmitted once (issue 278)
    size_t triggeredModelEvaluated = 0, triggeredNodeEvaluated = 0;
    QObject::connect(&model, &GraphExecutionModel::nodeEvaluated, &graph, [&](NodeUuid id){
        assert(id == A_uuid),
        triggeredModelEvaluated++;
    });
    QObject::connect(A, &Node::evaluated, &graph, [&](){
        triggeredNodeEvaluated++;
    });

    EXPECT_EQ(triggeredModelEvaluated, 0);
    EXPECT_EQ(triggeredNodeEvaluated, 0);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, C_uuid, D_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";

    auto future = model.evaluateNode(A_uuid);
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";

    EXPECT_TRUE(model.isNodeEvaluated(A_uuid));
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
        // only node A should be evaluated and valid
        {A_uuid, NodeEvalState::Valid},
        // all other nodes are still outdated
        {B_uuid, NodeEvalState::Outdated},
        {C_uuid, NodeEvalState::Outdated},
        {D_uuid, NodeEvalState::Outdated}
    }));

    constexpr double EXPECTED_VALUE = 42.0;
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
        // only port 0 is connected
        {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
        // all other ports are still outdated and have no data associated
        {B_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, {}},
        // all other nodes are not evaluated
        {C_uuid, PortType::In , PortIndex(0), PortDataState::Outdated, {}},
        {C_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
        {C_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, {}},
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Outdated, {}}
    }));

    EXPECT_EQ(triggeredModelEvaluated, 1);
    EXPECT_EQ(triggeredNodeEvaluated, 1);
}

/// Evaluate a single node that has dependencies on the same graph level
TEST(GraphExecutionModel, evaluate_node_with_dependencies)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";

    auto future = model.evaluateNode(D_uuid);
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";

    EXPECT_TRUE(model.isNodeEvaluated(D_uuid));
    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
        {A_uuid, NodeEvalState::Valid},
        {B_uuid, NodeEvalState::Valid},
        {C_uuid, NodeEvalState::Valid},
        {D_uuid, NodeEvalState::Valid}
    }));

    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_B = EXPECTED_VALUE_A;
    constexpr double EXPECTED_VALUE_C = EXPECTED_VALUE_B * 2;

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {C_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {C_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C}
    }));
}

/// Evaluate a single node that has dependencies across different graph levels
TEST(GraphExecutionModel, evaluate_node_with_nested_dependencies)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";

    auto future = model.evaluateNode(D_uuid);
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);
    gtTrace() << "Validate results...";

    EXPECT_TRUE(model.isNodeEvaluated(D_uuid));
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
         {A_uuid, NodeEvalState::Valid},
         {B_uuid, NodeEvalState::Valid},
         {C_uuid, NodeEvalState::Valid},
         {D_uuid, NodeEvalState::Valid},
         // other nodes are still outdated
         {E_uuid, NodeEvalState::Outdated}
    }));

    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B =  8.0;
    constexpr double EXPECTED_VALUE_C = EXPECTED_VALUE_A + EXPECTED_VALUE_B * 2;

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {C_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {C_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {C_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, {}},
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B}
    }));
}

/// Evaluate a graph with a single nested layer (depth of 1). Evaluate both
/// the root and subgraph
TEST(GraphExecutionModel, evaluate_graph_with_single_layer)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    Graph* subgraph = graph.graphNodes().at(0);
    ASSERT_TRUE(subgraph);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
             group_uuid, group_input_uuid, group_output_uuid,
             group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate root graph...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results of root graph...";

    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {group_uuid, NodeEvalState::Valid},
            {group_input_uuid, NodeEvalState::Valid},
            {group_output_uuid, NodeEvalState::Valid},
            {group_A_uuid, NodeEvalState::Valid},
            {group_B_uuid, NodeEvalState::Valid},
            {group_C_uuid, NodeEvalState::Valid},
            // all nodes in the root graph have been evaluated
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid},
            {D_uuid, NodeEvalState::Valid},
            {E_uuid, NodeEvalState::Valid},
            // nodes in subgraph that are not required are not evaluated
            {group_D_uuid, NodeEvalState::Outdated},
        }));

    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B =  8.0;
    constexpr double EXPECTED_VALUE_C = 42.0;
    constexpr double EXPECTED_VALUE_D = EXPECTED_VALUE_C + EXPECTED_VALUE_B;

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
            // A was evaluated and has the expected value
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            // B is connected to E, both should share the same value
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            // graphs inputs are correct
            {group_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {group_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            {group_input_uuid, PortType::Out , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {group_input_uuid, PortType::Out , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            // graph outputs are correct
            {group_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {group_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, {}}, // TODO: should this data be considered valid?
            {group_output_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {group_output_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
            // D was evaluated correctly
            {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {D_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            {D_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D}
        }));

    gtTrace() << "Reset...";

    model.reset();

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, PortDataState::Outdated, {nullptr}));

    debug(model);

    gtTrace() << "Evaluate subgraph only...";

    future = model.evaluateGraph(*subgraph);
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results of sub graph...";

    EXPECT_TRUE(model.isGraphEvaluated(*subgraph));

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            // all nodes in the subgraph are evaluated
            {group_uuid, NodeEvalState::Valid},
            {group_input_uuid, NodeEvalState::Valid},
            {group_output_uuid, NodeEvalState::Valid},
            {group_A_uuid, NodeEvalState::Valid},
            {group_B_uuid, NodeEvalState::Valid},
            {group_C_uuid, NodeEvalState::Valid},
            {group_D_uuid, NodeEvalState::Valid},
            // only successors are evaluated
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid},
            {D_uuid, NodeEvalState::Outdated},
            {E_uuid, NodeEvalState::Outdated},
        }));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
            // A was evaluated and has the expected value
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            // B is connected to E, both should share the same value
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            // graphs inputs are correct
            {group_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {group_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            {group_input_uuid, PortType::Out , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {group_input_uuid, PortType::Out , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            // graph outputs are correct
            {group_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {group_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, {}}, // TODO: should this data be considered valid?
            {group_output_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {group_output_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
            // D was not evaluated
            {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {D_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
            {D_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, {}}
        }));
}

/// Evaluate a graph with a subgraph which directly forwards the input data
/// to its outputs
TEST(GraphExecutionModel, evaluate_graph_with_forwarding_layer)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithForwardingGroup(graph));

    auto* group = qobject_cast<Graph*>(graph.findNode(group_id));
    ASSERT_TRUE(group);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid
        }, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid
        }, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";

    // all nodes should be evaluated and valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, D_uuid, E_uuid,
         group_uuid, group_input_uuid, group_output_uuid},
        NodeEvalState::Valid));

    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B =  8.0;
    constexpr double EXPECTED_VALUE_D = EXPECTED_VALUE_A + EXPECTED_VALUE_B;

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        // A was evaluated and has the expected value
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        // B is connected to E, both should share the same value
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        // group forwards input data to its outputs
        {group_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {group_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {group_input_uuid, PortType::Out , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_input_uuid, PortType::Out , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {group_output_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_output_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        // D was evaluated correctly
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {D_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {D_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D}
    }));
}

/// Evaluate a graph in which two separate data flows exists
TEST(GraphExecutionModel, evaluate_graph_with_separate_flows)
{
    Graph graph;

    GraphBuilder builder(graph);
    // source nodes
    Node& A1 = builder.addNode("TestNumberInputNode", "A1_uuid").setCaption("A1");
    Node& A2 = builder.addNode("TestNumberInputNode", "A2_uuid").setCaption("A2");

    Node& B1 = builder.addNode("intelli::NumberMathNode", "B1_uuid").setCaption("B1");
    Node& B2 = builder.addNode("intelli::NumberMathNode", "B2_uuid").setCaption("B2");

    setNodeProperty(A1, QStringLiteral("value"), 42);
    setNodeProperty(A2, QStringLiteral("value"), 42);

    setNodeProperty(B1, QStringLiteral("operation"), QStringLiteral("Plus"));
    setNodeProperty(B2, QStringLiteral("operation"), QStringLiteral("Plus"));

    builder.connect(A1, PortIndex(0), B1, PortIndex(0));
    builder.connect(A1, PortIndex(0), B1, PortIndex(1));

    builder.connect(A2, PortIndex(0), B2, PortIndex(0));
    builder.connect(A2, PortIndex(0), B2, PortIndex(1));

    debug(graph);

    GraphExecutionModel model(graph);

    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A1.uuid(), B1.uuid(), A2.uuid(), B2.uuid()},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A1.uuid(), B1.uuid(), A2.uuid(), B2.uuid()},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.startedSuccessfully());
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";

    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
        {A1.uuid(), NodeEvalState::Valid},
        {A2.uuid(), NodeEvalState::Valid},
        {B1.uuid(), NodeEvalState::Valid},
        {B2.uuid(), NodeEvalState::Valid}
    }));

    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_B = 2 * EXPECTED_VALUE_A;
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        // 1st graph
        {A1.uuid(), PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B1.uuid(), PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B1.uuid(), PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_A},
        {B1.uuid(), PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        // 2nd graph
        {A2.uuid(), PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B2.uuid(), PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B2.uuid(), PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_A},
        {B2.uuid(), PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
    }));
}

/// During the evaluation of a graph append a new node and connected it to the
/// existing flow. It will not be evaluated as only the nodes are executed that
/// were present when triggering the graph evaluation.
TEST(GraphExecutionModel, evaluate_graph_with_node_appended)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    gtTrace() << "Evalauting...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.startedSuccessfully());

    bool functionCalled = false;

    gtTrace() << "Scheduling callback function...";

    model.evaluateNode(A_uuid).then([&graph, &functionCalled](bool success){
        gtTrace() << "Callback function called!";
        functionCalled = true;

        ASSERT_TRUE(success);

        GraphBuilder builder(graph);
        builder.addNode("intelli::NumberDisplayNode", E_uuid).setCaption("E");

        builder.connect(C_id, PortIndex(0), E_id, PortIndex(0));
    });

    EXPECT_TRUE(future.wait(maxTimeout));
    ASSERT_TRUE(functionCalled);

    debug(graph);
    debug(model);

    gtTrace() << "Validating...";

    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
        {A_uuid, NodeEvalState::Valid},
        {B_uuid, NodeEvalState::Valid},
        {C_uuid, NodeEvalState::Valid},
        {D_uuid, NodeEvalState::Valid},
        {E_uuid, NodeEvalState::Outdated}
    }));

    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_D = EXPECTED_VALUE_A * 2;
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D},
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D},
        {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D}
    }));
}

/// During the evaluation of a graph append a new connection that affects
/// a target node -> Reevaluate affected nodes
TEST(GraphExecutionModel, evaluate_graph_with_connection_appended)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    gtTrace() << "Evalauting...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.startedSuccessfully());

    bool functionCalled = false;

    gtTrace() << "Scheduling callback function...";

    model.evaluateNode(B_uuid).then([&graph, &functionCalled](bool success){
        gtTrace() << "Callback function called!";
        functionCalled = true;

        ASSERT_TRUE(success);

        GraphBuilder builder(graph);
        builder.connect(A_id, PortIndex(0), B_id, PortIndex(1));
    });

    EXPECT_TRUE(future.wait(maxTimeout));
    ASSERT_TRUE(functionCalled);

    debug(graph);
    debug(model);

    gtTrace() << "Validating...";

    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid},
            {C_uuid, NodeEvalState::Valid},
            {D_uuid, NodeEvalState::Valid}
        }));

    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_B = EXPECTED_VALUE_A * 2;
    constexpr double EXPECTED_VALUE_C = EXPECTED_VALUE_B * 2;
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        }));
}

TEST(GraphExecutionModel, evaluate_graph_with_connection_deleted)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    gtTrace() << "Evalauting...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.startedSuccessfully());

    bool functionCalled = false;

    gtTrace() << "Scheduling callback function...";

    model.evaluateNode(B_uuid).then([&graph, &functionCalled](bool success){
        gtTrace() << "Callback function called!";
        functionCalled = true;

        ASSERT_TRUE(success);

        ConnectionId conId = graph.connectionId(A_id, PortIndex(0), B_id, PortIndex(0));
        ASSERT_TRUE(conId.isValid());
        ASSERT_TRUE(graph.deleteConnection(conId));
    });

    EXPECT_TRUE(future.wait(maxTimeout));
    ASSERT_TRUE(functionCalled);

    debug(graph);
    debug(model);

    gtTrace() << "Validating...";

    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid},
            {C_uuid, NodeEvalState::Valid},
            {D_uuid, NodeEvalState::Valid}
        }));

    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_B = 0;
    constexpr double EXPECTED_VALUE_C = EXPECTED_VALUE_B * 2;
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
            {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
            {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        }));
    }

/// Evaluating a graph with an paused node succeeds, as a paused node is
/// only relevant for auto evaluation of the graph
TEST(GraphExecutionModel, evaluate_graph_with_paused_node)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    Node* B = graph.findNode(B_id);
    B->setActive(false);
    ASSERT_TRUE(B);

    gtTrace() << "Evalauting...";

    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));

    gtTrace() << "Validate...";
    EXPECT_TRUE(model.isGraphEvaluated());

    debug(model);
}

/// If a nodes recieves new input data or was invalidated it and all successor
/// nodes should be invalidated (=outdated) as well
TEST(GraphExecutionModel, propagate_invalidation)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    Node* nodeA = graph.findNode(A_id);
    ASSERT_TRUE(nodeA);
    Node* nodeB = graph.findNode(B_id);
    ASSERT_TRUE(nodeB);

    debug(graph);
    debug(model);

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
        NodeEvalState::Outdated));

    gtDebug() << "Setting node data of node A...";

    constexpr double EXPECTED_VALUE = 42.0;
    auto dataPtr = std::make_shared<DoubleData const>(EXPECTED_VALUE);

    NodeDataSet dataA;
    dataA.ptr = dataPtr;
    dataA.state = PortDataState::Valid;

    ASSERT_TRUE(model.setNodeData(A_uuid, PortType::Out, PortIndex(0), dataPtr));

    gtDebug() << "Triggering evaluation of node A...";

    /// evaluate node A once -> make data valid
    ASSERT_TRUE(exec::blockingEvaluation(*nodeA, model));

    {
        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
        }));

        EXPECT_TRUE(test::comparePortData<double>(graph, model, {
             {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
             {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
             {B_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
             {B_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, {}}
        }));
    }

    gtDebug() << "Triggering evaluation of node B...";

    /// evaluate node B once -> make data valid
    ASSERT_TRUE(exec::blockingEvaluation(*nodeB, model));

    {
        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid}
        }));

        EXPECT_TRUE(test::comparePortData<double>(graph, model, {
            /// data is set and valid
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE}
        }));
    }

    gtDebug() << "Setting node data of node B...";

    /// set new input data -> make node outdated
    ASSERT_TRUE(model.setNodeData(B_uuid, PortType::In, PortIndex(1), dataPtr));

    {
        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            /// only node B is now outdated
            {B_uuid, NodeEvalState::Outdated}
        }));

        EXPECT_TRUE(test::comparePortData<double>(graph, model, {
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE},
            /// out data is outdated and has old value
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, EXPECTED_VALUE}
        }));
    }

    gtDebug() << "Triggering evaluation of node B...";

    /// evaluate node B once -> make data valid
    ASSERT_TRUE(exec::blockingEvaluation(*nodeB, model));

    {
        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid}
        }));

        EXPECT_TRUE(test::comparePortData<double>(graph, model, {
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE * 2}
        }));
    }

    gtDebug() << "Invalidating...";

    /// invalidate node A -> make output data and successors outdated
    ASSERT_TRUE(model.invalidateNode(A_uuid));

    {
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid},
            NodeEvalState::Outdated));

        EXPECT_TRUE(test::comparePortData<double>(graph, model, {
            {A_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(0), PortDataState::Outdated, EXPECTED_VALUE},
            {B_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE},
            {B_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, EXPECTED_VALUE * 2}
        }));
    }
}

/// If a nodes fails during evaluation all successor nodes are marked as failed
/// as well.
TEST(GraphExecutionModel, propagate_failed_evaluation)
{
    Graph graph;

    ASSERT_TRUE(test::buildLinearGraph(graph));

    ASSERT_TRUE(graph.deleteNode(B_id));

    gtDebug() << "Setup...";

    // setup test node to allow toggling whether node succeeds
    auto testNodePtr = std::make_unique<TestNode>();
    testNodePtr->setCaption("B");
    testNodePtr->setId(B_id);
    testNodePtr->setUuid(B_uuid);
    testNodePtr->setNodeEvalMode(NodeEvalMode::Blocking);

    TestNode* testNode = graph.appendNode(std::move(testNodePtr), NodeIdPolicy::Keep);
    ASSERT_TRUE(testNode);

    PortId in = testNode->addInPort(typeId<DoubleData>());
    PortId out = testNode->addOutPort(typeId<DoubleData>());
    ASSERT_TRUE(in.isValid());
    ASSERT_TRUE(out.isValid());

    {
        GraphBuilder builder(graph);
        builder.connect(A_id, PortIndex(0), testNode->id(), PortIndex(0));
        builder.connect(testNode->id(), PortIndex(0), C_id, PortIndex(0));
    }

    debug(graph);

    GraphExecutionModel model(graph);

    EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
        {A_uuid, NodeEvalState::Outdated},
        {B_uuid, NodeEvalState::Outdated},
        {C_uuid, NodeEvalState::Outdated},
        {D_uuid, NodeEvalState::Outdated}
    }));

    gtDebug() << "Triggering evaluation of graph...";

    testNode->failEvaluation = false;
    auto future = model.evaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));

    {
        EXPECT_TRUE(model.isGraphEvaluated());
        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
             {A_uuid, NodeEvalState::Valid},
             {B_uuid, NodeEvalState::Valid},
             {C_uuid, NodeEvalState::Valid},
             {D_uuid, NodeEvalState::Valid}
         }));
    }

    gtDebug() << "Invalidating...";

    testNode->failEvaluation = true;
    model.invalidateNode(B_uuid);

    {
        EXPECT_FALSE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Outdated},
            {C_uuid, NodeEvalState::Outdated},
            {D_uuid, NodeEvalState::Outdated}
        }));
    }

    gtDebug() << "Triggering evaluation of graph #2...";

    // test node fails -> all successors are marked as failed as well
    future = model.evaluateGraph();
    EXPECT_FALSE(future.wait(maxTimeout));

    {
        EXPECT_FALSE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Invalid},
            {C_uuid, NodeEvalState::Invalid},
            {D_uuid, NodeEvalState::Invalid}
        }));
    }

    // model may still nodes marked for evaluation
    model.resetTargetNodes();

    gtDebug() << "Deleting connection...";

    // delete connection with invalid node
    ASSERT_TRUE(graph.deleteConnection(
        graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0))));

    {
        EXPECT_FALSE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Invalid},
            {C_uuid, NodeEvalState::Outdated},
            {D_uuid, NodeEvalState::Outdated}
        }));
    }

    gtDebug() << "Reconnecting...";

    // reconnect
    {
        GraphBuilder builder(graph);
        builder.connect(B_id, PortIndex(0), C_id, PortIndex(0));

        EXPECT_FALSE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Invalid},
            {C_uuid, NodeEvalState::Invalid},
            {D_uuid, NodeEvalState::Invalid}
        }));
    }

    gtDebug() << "Invalidating #2...";

    // Invalidating a node will make all its successor nodes become outdated again
    model.invalidateNode(B_uuid);

    {
        EXPECT_FALSE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(graph, model, {
             {A_uuid, NodeEvalState::Valid},
             {B_uuid, NodeEvalState::Outdated},
             {C_uuid, NodeEvalState::Outdated},
             {D_uuid, NodeEvalState::Outdated}
         }));
    }
}

/// The auto evaluation is tested in on a graph that has one subgraph.
/// Both the root and subgraph are tested separatedly.
TEST(GraphExecutionModel, auto_evaluate_graph_with_single_layer)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    Node* B = graph.findNode(B_id);
    ASSERT_TRUE(B);
    Graph* subgraph = graph.graphNodes().at(0);
    ASSERT_TRUE(subgraph);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Auto evaluate root graph...";

    EXPECT_TRUE(model.autoEvaluateGraph());

    GtEventLoop loop{maxTimeout};
    loop.exec();

    debug(model);

    gtTrace() << "Validate results of root graph...";

    EXPECT_TRUE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {group_uuid, NodeEvalState::Valid},
            {group_input_uuid, NodeEvalState::Valid},
            {group_output_uuid, NodeEvalState::Valid},
            {group_A_uuid, NodeEvalState::Valid},
            {group_B_uuid, NodeEvalState::Valid},
            {group_C_uuid, NodeEvalState::Valid},
            // all nodes in the root graph have been evaluated
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Valid},
            {D_uuid, NodeEvalState::Valid},
            {E_uuid, NodeEvalState::Valid},
            // nodes in subgraph that are not required are not evaluated
            {group_D_uuid, NodeEvalState::Outdated},
        }));

    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B =  8.0;
    constexpr double EXPECTED_VALUE_C = 42.0;
    constexpr double EXPECTED_VALUE_D = EXPECTED_VALUE_C + EXPECTED_VALUE_B;

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        // A was evaluated and has the expected value
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        // B is connected to E, both should share the same value
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        // graphs inputs are correct
        {group_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {group_input_uuid, PortType::Out , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_input_uuid, PortType::Out , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        // graph outputs are correct
        {group_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {group_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, {}}, // TODO: should this data be considered valid?
        {group_output_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {group_output_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
        // D was evaluated correctly
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {D_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {D_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_D}
    }));

    gtTrace() << "Stopping auto evaluation...";

    model.stopAutoEvaluatingGraph();
    emit B->triggerNodeEvaluation();

    EXPECT_FALSE(model.isEvaluating());

    gtTrace() << "Validating...";

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Outdated},
            {D_uuid, NodeEvalState::Outdated},
            {E_uuid, NodeEvalState::Outdated},
            {group_uuid, NodeEvalState::Outdated},
            {group_input_uuid, NodeEvalState::Outdated},
            {group_output_uuid, NodeEvalState::Outdated},
            {group_A_uuid, NodeEvalState::Valid},
            {group_B_uuid, NodeEvalState::Valid},
            {group_C_uuid, NodeEvalState::Outdated},
            {group_D_uuid, NodeEvalState::Outdated},
        }));

    gtTrace() << "Reset...";

    model.reset();

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, PortDataState::Outdated, {nullptr}));

    debug(model);

    gtTrace() << "Auto evaluate subgraph only...";

    EXPECT_TRUE(model.autoEvaluateGraph(*subgraph));
    loop.exec();

    debug(model);

    gtTrace() << "Validate results of sub graph...";

    EXPECT_TRUE(model.isGraphEvaluated(*subgraph));

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
        // all nodes in the subgraph are evaluated
        {group_uuid, NodeEvalState::Valid},
        {group_input_uuid, NodeEvalState::Valid},
        {group_output_uuid, NodeEvalState::Valid},
        {group_A_uuid, NodeEvalState::Valid},
        {group_B_uuid, NodeEvalState::Valid},
        {group_C_uuid, NodeEvalState::Valid},
        {group_D_uuid, NodeEvalState::Valid},
        // only successors are evaluated
        {A_uuid, NodeEvalState::Valid},
        {B_uuid, NodeEvalState::Valid},
        {D_uuid, NodeEvalState::Outdated},
        {E_uuid, NodeEvalState::Outdated}
    }));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {
        // A was evaluated and has the expected value
        {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        // B is connected to E, both should share the same value
        {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
        // graphs inputs are correct
        {group_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {group_input_uuid, PortType::Out , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
        {group_input_uuid, PortType::Out , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        // graph outputs are correct
        {group_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {group_uuid, PortType::Out, PortIndex(1), PortDataState::Valid, {}}, // TODO: should this data be considered valid?
        {group_output_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {group_output_uuid, PortType::In , PortIndex(1), PortDataState::Outdated, {}},
        // D was not evaluated
        {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
        {D_uuid, PortType::In , PortIndex(1), PortDataState::Valid, EXPECTED_VALUE_B},
        {D_uuid, PortType::Out, PortIndex(0), PortDataState::Outdated, {}}
    }));

    gtTrace() << "Stopping auto evaluation of subgraph...";

    model.stopAutoEvaluatingGraph(*subgraph);
    emit B->triggerNodeEvaluation();

    EXPECT_FALSE(model.isEvaluating());

    gtTrace() << "Validating...";

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Outdated},
            {D_uuid, NodeEvalState::Outdated},
            {E_uuid, NodeEvalState::Outdated},
            // all nodes in the subgraph are now outdated
            {group_uuid, NodeEvalState::Outdated},
            {group_input_uuid, NodeEvalState::Outdated},
            {group_output_uuid, NodeEvalState::Outdated},
            {group_A_uuid, NodeEvalState::Valid},
            {group_B_uuid, NodeEvalState::Valid},
            {group_C_uuid, NodeEvalState::Outdated},
            {group_D_uuid, NodeEvalState::Outdated},
        }));
}

/// A more complex modification is applied to a graph and the auto evaluation
/// is tested. Following changes are made:
///  1. first a node is appended
///  2. connections between two nodes are separated, such that two separate
///     flows are created
///  3. all changes are reverted using a memento diff
TEST(GraphExecutionModel, auto_evaluate_graph_with_memento_diff)
{
    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_B = EXPECTED_VALUE_A;
    constexpr double EXPECTED_VALUE_C = EXPECTED_VALUE_B * 2;
    constexpr double EXPECTED_VALUE_NULL = {};

    GtEventLoop loop{maxTimeout};

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Creating memento...";
    GtObjectMemento mementoBefore = graph.toMemento();
    ASSERT_FALSE(mementoBefore.isNull());

    // initial graph
    {
        // all nodes should be outdated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid
            }, NodeEvalState::Outdated));

        // all ports should be outdated and have no data associated
        EXPECT_TRUE(test::comparePortData(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid
            }, PortDataState::Outdated, {nullptr}));

        gtTrace() << "Initial evaluation...";

        EXPECT_TRUE(model.autoEvaluateGraph());

        loop.exec();

        debug(model);

        gtTrace() << "Validating initial evaluation...";

        EXPECT_TRUE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid
            }, NodeEvalState::Valid));
        // node E does not exist
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                E_uuid
            }, NodeEvalState::Invalid));

        EXPECT_TRUE(test::comparePortData<double>(
            graph, model, {
                {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
                {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
                {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
                {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C}
            }));
    }

    // appending node
    {
        gtTrace() << "Appending node...";

        GraphBuilder builder(graph);
        builder.addNode("intelli::NumberDisplayNode", E_uuid).setCaption("E");
        builder.connect(B_id, PortIndex(0), E_id, PortIndex(0));

        loop.exec();

        debug(graph);
        debug(model);

        gtTrace() << "Validating after appending node...";

        EXPECT_TRUE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid, E_uuid
            }, NodeEvalState::Valid));

        EXPECT_TRUE(test::comparePortData<double>(
            graph, model, {
                {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
                {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
                {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
                {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
                {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B}
            }));
    }

    // deleting connections
    {
        gtTrace() << "Deleting connections...";

        ConnectionId conId1 = graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0));
        ConnectionId conId2 = graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(1));
        ASSERT_TRUE(conId1.isValid());
        ASSERT_TRUE(graph.deleteConnection(conId1));
        ASSERT_TRUE(conId2.isValid());
        ASSERT_TRUE(graph.deleteConnection(conId2));

        loop.exec();

        debug(graph);
        debug(model);

        gtTrace() << "Validating after deleting connections...";

        EXPECT_TRUE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid, E_uuid
            }, NodeEvalState::Valid));

        EXPECT_TRUE(test::comparePortData<double>(
            graph, model, {
                {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
                {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
                {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_NULL},
                {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_NULL},
                {E_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B}
            }));
    }

    gtTrace() << "Creating memento...";
    GtObjectMemento mementoAfter= graph.toMemento();
    ASSERT_FALSE(mementoAfter.isNull());

    // apply memento diff
    {
        gtTrace() << "Reverting memento...";
        GtObjectMementoDiff diff{mementoBefore, mementoAfter};
        ASSERT_TRUE(graph.revertDiff(diff));

        loop.exec();

        debug(graph);
        debug(model);

        gtTrace() << "Validating after memento diff...";

        EXPECT_TRUE(model.isGraphEvaluated());

        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A_uuid, B_uuid, C_uuid, D_uuid
            }, NodeEvalState::Valid));
        // node E has been deleted
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {
                E_uuid
            }, NodeEvalState::Invalid));

        EXPECT_TRUE(test::comparePortData<double>(
            graph, model, {
                {A_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_A},
                {B_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_B},
                {C_uuid, PortType::Out, PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C},
                {D_uuid, PortType::In , PortIndex(0), PortDataState::Valid, EXPECTED_VALUE_C}
            }));
    }
}

/// Evaluating a graph with paused nodes only evaluates all nodes that are not
/// paused/inactive.
TEST(GraphExecutionModel, auto_evaluate_graph_with_paused_subgraph)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    Node* B = graph.findNode(B_id);
    B->setActive(false);
    ASSERT_TRUE(B);

    gtTrace() << "Evalauting...";

    EXPECT_TRUE(model.autoEvaluateGraph());

    GtEventLoop loop{maxTimeout};
    loop.exec();

    gtTrace() << "Validate...";
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            {A_uuid, NodeEvalState::Valid},
            {B_uuid, NodeEvalState::Paused},
            {C_uuid, NodeEvalState::Outdated},
            {D_uuid, NodeEvalState::Outdated}
        }));
}

/// Append a subgraph to a graph that has auto evaluation enabled
TEST(GraphExecutionModel, auto_evaluate_graph_wihle_appending_subgraph)
{
    // creating temporary subgraph
    gtTrace() << "Creating subgraph...";

    auto subgraph  = std::make_unique<Graph>();
    subgraph->setCaption("Subgraph");
    subgraph->initInputOutputProviders();

    {
        auto* inputNode = subgraph->inputProvider();
        auto* outputNode = subgraph->outputProvider();
        ASSERT_TRUE(inputNode);
        ASSERT_TRUE(outputNode);

        ASSERT_TRUE(inputNode->addPort(typeId<DoubleData>()).isValid());
        ASSERT_TRUE(outputNode->addPort(typeId<DoubleData>()).isValid());

        GraphBuilder builder(*subgraph);
        auto change = subgraph->modify();
        Node& graphNodeA = builder.addNode("intelli::NumberMathNode");
        builder.connect(*inputNode, PortIndex(0), graphNodeA, PortIndex(0));
        builder.connect(graphNodeA, PortIndex(0), *outputNode, PortIndex(0));
        change.finalize();
    }

    // creating root graph
    gtTrace() << "Creating root graph...";
    Graph root;

    GraphExecutionModel model(root);
    EXPECT_TRUE(model.autoEvaluateGraph());

    test::buildLinearGraph(root);

    gtTrace() << "Appending subgraph...";
    EXPECT_TRUE(root.appendNode(std::move(subgraph)));

    EXPECT_FALSE(model.isGraphEvaluated());

    gtTrace() << "Waiting for auto evaluation...";
    GtEventLoop loop{maxTimeout};
    loop.exec();

    gtTrace() << "Validating...";
    EXPECT_TRUE(model.isGraphEvaluated());
}

/// Evalauting a nodes that is "exclusive" should be evaluated separatly to
/// all other nodes.
TEST(GraphExecutionModel, evaluation_of_exclusive_nodes)
{
    std::chrono::seconds maxTimeout(4);

    Graph graph;

    GraphBuilder builder(graph);

    Node& S = builder.addNode(QStringLiteral("TestNumberInputNode"), "S_UUID")
                  .setCaption("S");
    TestSleepyNode& A = builder.addNode<TestSleepyNode>(A_uuid);
    A.setCaption("A");
    TestSleepyNode& B = builder.addNode<TestSleepyNode>(B_uuid);
    B.setCaption("B");
    TestSleepyNode& C = builder.addNode<TestSleepyNode>(C_uuid);
    C.setCaption("C");
    Node& T1 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), "T1_UUID")
                   .setCaption("T1");
    Node& T2 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), "T2_UUID")
                   .setCaption("T2");
    Node& T3 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), "T3_UUID")
                   .setCaption("T3");

    builder.connect(S, PortIndex(0), A, PortIndex(0));
    builder.connect(A, PortIndex(0), T1, PortIndex(0));

    builder.connect(S, PortIndex(0), B, PortIndex(0));
    builder.connect(B, PortIndex(0), T2, PortIndex(0));

    builder.connect(S, PortIndex(0), C, PortIndex(0));
    builder.connect(C, PortIndex(0), T3, PortIndex(0));

    setNodeProperty(A, "timer", 1);
    setNodeProperty(B, "timer", 1);
    setNodeProperty(C, "timer", 1);

    A.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);
    B.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);
    C.setNodeEvalMode(NodeEvalMode::Detached); // should still run exclusively

    GraphExecutionModel model(graph);

    int nodeEvaluations = 0;
    bool callbackFunctionCalled = false;

    auto checkState = [&](NodeUuid const& nodeUuid){
        auto state = model.nodeEvalState(nodeUuid);
        ASSERT_NE(state, NodeEvalState::Invalid);

        if (state != NodeEvalState::Evaluating ||
            (nodeUuid != A.uuid() &&
             nodeUuid != B.uuid() &&
             nodeUuid != C.uuid())) return;

        auto stateA = model.nodeEvalState(A.uuid());
        auto stateB = model.nodeEvalState(B.uuid());
        auto stateC = model.nodeEvalState(C.uuid());

        EXPECT_TRUE(stateA == NodeEvalState::Evaluating ^
                    stateB == NodeEvalState::Evaluating ^
                    stateC == NodeEvalState::Evaluating);
        nodeEvaluations++;
    };

    QObject::connect(&A, &Node::nodeEvalStateChanged,
                     &model, std::bind(checkState, A.uuid()));
    QObject::connect(&B, &Node::nodeEvalStateChanged,
                     &model, std::bind(checkState, B.uuid()));
    QObject::connect(&C, &Node::nodeEvalStateChanged,
                     &model, std::bind(checkState, C.uuid()));

    debug(graph);

    gtTrace() << "Evaluate...";

    auto future = model.evaluateGraph();

    // no test node has started yet
    model.evaluateNode(S.uuid()).then([&](bool success){
        gtTrace() << "On Node S evaluated...";
        callbackFunctionCalled = true;

        ASSERT_TRUE(success);
        ASSERT_TRUE(test::compareNodeEvalState(
            graph, model, {S.uuid()}, NodeEvalState::Valid));

        ASSERT_TRUE(test::compareNodeEvalState(
            graph, model, {
                A.uuid(), B.uuid(), C.uuid(),
                T1.uuid(), T2.uuid(), T3.uuid()
            }, NodeEvalState::Outdated));
    });

    EXPECT_TRUE(future.wait(maxTimeout));

    gtTrace() << "Validate Results...";

    EXPECT_EQ(nodeEvaluations, 3);
    EXPECT_TRUE(callbackFunctionCalled);

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            S.uuid(),
            A.uuid(), B.uuid(), C.uuid(),
            T1.uuid(), T2.uuid(), T3.uuid()
        }, NodeEvalState::Valid));
}

/// Evalauting a nodes that is "exclusive" should be evaluated separatly to
/// all other nodes including all other nodes in other graph execution models
/// (exclusive nodes are considered globally exlcusive).
TEST(GraphExecutionModel, evaluation_of_exclusive_nodes_across_multiple_graphs)
{
    std::chrono::seconds maxTimeout(10);

    QVector<QStringList> nodeMap;

    auto setupGraph = [&](Graph& g){
        GraphBuilder builder(g);

        Node& S = builder.addNode(QStringLiteral("TestNumberInputNode"), g.caption() +  "S_UUID")
                      .setCaption("S");
        TestSleepyNode& A = builder.addNode<TestSleepyNode>(g.caption() + A_uuid);
        A.setCaption("A");
        TestSleepyNode& B = builder.addNode<TestSleepyNode>(g.caption() + B_uuid);
        B.setCaption("B");
        TestSleepyNode& C = builder.addNode<TestSleepyNode>(g.caption() + C_uuid);
        C.setCaption("C");
        Node& T1 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), g.caption() +  "T1_UUID")
                       .setCaption("T1");
        Node& T2 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), g.caption() +  "T2_UUID")
                       .setCaption("T2");
        Node& T3 = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), g.caption() +  "T3_UUID")
                       .setCaption("T3");

        builder.connect(S, PortIndex(0), A, PortIndex(0));
        builder.connect(A, PortIndex(0), T1, PortIndex(0));

        builder.connect(S, PortIndex(0), B, PortIndex(0));
        builder.connect(B, PortIndex(0), T2, PortIndex(0));

        builder.connect(S, PortIndex(0), C, PortIndex(0));
        builder.connect(C, PortIndex(0), T3, PortIndex(0));

        setNodeProperty(A, "timer", 1);
        setNodeProperty(B, "timer", 1);
        setNodeProperty(C, "timer", 1);

        A.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);
        B.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);
        C.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);

        nodeMap.push_back({A.uuid(), B.uuid(), C.uuid()});
    };

    Graph graph1, graph2;
    graph1.setCaption("Graph1");
    graph2.setCaption("Graph2");
    setupGraph(graph1);
    setupGraph(graph2);

    ASSERT_EQ(nodeMap.size(), 2);

    GraphExecutionModel model1(graph1);
    GraphExecutionModel model2(graph2);

    int nodeEvaluations = 0;

    auto checkState = [&](NodeUuid const& uuid){
        if (!nodeMap[0].contains(uuid) && !nodeMap[1].contains(uuid)) return;

        auto numberOfNodesEvaluating = [](auto const& model,
                                          auto const& nodes) -> bool{
            return std::count_if(nodes.begin(), nodes.end(), [&model](NodeUuid const& uuid) -> bool{
                return model.nodeEvalState(uuid) == NodeEvalState::Evaluating;
            });
        };

        int nodesEvaluatingInModel1 = numberOfNodesEvaluating(model1, nodeMap[0]);
        int nodesEvaluatingInModel2 = numberOfNodesEvaluating(model2, nodeMap[1]);

        EXPECT_LE(nodesEvaluatingInModel1, 1);
        EXPECT_LE(nodesEvaluatingInModel2, 1);
        EXPECT_LE(nodesEvaluatingInModel1 + nodesEvaluatingInModel2, 1);

        if ((nodesEvaluatingInModel1 + nodesEvaluatingInModel2) == 1) nodeEvaluations++;
    };


    QObject::connect(graph1.findNodeByUuid(nodeMap[0][0]), &Node::nodeEvalStateChanged,
                     &model1, std::bind(checkState, nodeMap[0][0]));
    QObject::connect(graph1.findNodeByUuid(nodeMap[0][1]), &Node::nodeEvalStateChanged,
                     &model1, std::bind(checkState, nodeMap[0][1]));
    QObject::connect(graph1.findNodeByUuid(nodeMap[0][2]), &Node::nodeEvalStateChanged,
                     &model1, std::bind(checkState, nodeMap[0][2]));

    QObject::connect(graph2.findNodeByUuid(nodeMap[1][0]), &Node::nodeEvalStateChanged,
                     &model2, std::bind(checkState, nodeMap[1][0]));
    QObject::connect(graph2.findNodeByUuid(nodeMap[1][1]), &Node::nodeEvalStateChanged,
                     &model2, std::bind(checkState, nodeMap[1][1]));
    QObject::connect(graph2.findNodeByUuid(nodeMap[1][2]), &Node::nodeEvalStateChanged,
                     &model2, std::bind(checkState, nodeMap[1][2]));

    gtTrace() << "Evaluate...";

    auto future1 = model1.evaluateGraph();
    auto future2 = model2.evaluateGraph();

    EXPECT_TRUE(future1.wait(maxTimeout) && future2.wait(maxTimeout));

    gtTrace() << "Validate Results...";

    EXPECT_EQ(nodeEvaluations, 6);

    EXPECT_TRUE(test::compareNodeEvalState(
        graph1, model1, {nodeMap[0]}, NodeEvalState::Valid));
    EXPECT_TRUE(model1.isGraphEvaluated());
    EXPECT_TRUE(test::compareNodeEvalState(
        graph2, model2, {nodeMap[1]}, NodeEvalState::Valid));
    EXPECT_TRUE(model2.isGraphEvaluated());
}

/// Attempting to evaluate a cyclic graph does not cause infinite loop but
/// simply fails
TEST(GraphExecutionModel, evaluation_of_cyclic_graph)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {
        auto& value1   = builder.addNode(QStringLiteral("TestNumberInputNode"), A_uuid).setCaption(QStringLiteral("A"));
        auto& value2   = builder.addNode(QStringLiteral("TestNumberInputNode"), B_uuid).setCaption(QStringLiteral("B"));

        auto& add1 = builder.addNode(QStringLiteral("intelli::NumberMathNode"), C_uuid).setCaption(QStringLiteral("C"));
        auto& add2 = builder.addNode(QStringLiteral("intelli::NumberMathNode"), D_uuid).setCaption(QStringLiteral("D"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), E_uuid).setCaption(QStringLiteral("E"));

        builder.connect(value1, PortIndex{0}, add1, PortIndex{0});
        builder.connect(add2,   PortIndex{0}, add1, PortIndex{1});

        builder.connect(add1,   PortIndex{0}, add2, PortIndex{0});
        builder.connect(value2, PortIndex{0}, add2, PortIndex{1});

        builder.connect(add2, PortIndex{0}, result, PortIndex{0});

        // set values
        setNodeProperty(value1, QStringLiteral("value"), 2);
        setNodeProperty(value2, QStringLiteral("value"), 10);

        setNodeProperty(add1, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(add2, QStringLiteral("operation"), QStringLiteral("Plus"));
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        ASSERT_NO_THROW(throw);
        return;
    }

    debug(graph);

    EXPECT_FALSE(isAcyclic(graph));

    GraphExecutionModel model(graph);

    EXPECT_FALSE(model.evaluateGraph().wait(maxTimeout));
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_FALSE(model.evaluateNode(E_uuid).wait(maxTimeout));

    EXPECT_FALSE(model.isGraphEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(E_uuid));

    EXPECT_TRUE(model.autoEvaluateGraph());

    GtEventLoop loop{maxTimeout};
    loop.exec();

    EXPECT_FALSE(model.isEvaluating());
    EXPECT_FALSE(model.isGraphEvaluated());
}

/// Destroying the graph exec model while its running should not cause any harm
TEST(GraphExecutionModel, destroy_while_running)
{
    Graph graph;

    {
        GraphExecutionModel model(graph);

        ASSERT_TRUE(test::buildLinearGraph(graph));

        model.evaluateGraph().detach();

        ASSERT_FALSE(model.isGraphEvaluated());

    } // model should still be evaluating
}

/// Model is owned by root graph. Should not cause any problems when
/// graph is beeing destroyed
TEST(GraphExecutionModel, destroy_when_deleting_root_graph)
{
    {
        auto graph = make_unique_qptr<Graph>();

        ASSERT_TRUE(test::buildGraphWithGroup(*graph));

        auto model = make_unique_qptr<GraphExecutionModel>(*graph);
        ASSERT_EQ(model->parent(), graph.get());

        graph->deleteLater();

        ASSERT_TRUE(graph);
        ASSERT_TRUE(model);

        GtEventLoop loop(directTimeout);
        loop.exec();

        ASSERT_FALSE(graph);
        ASSERT_FALSE(model);
    }
}

/// Accessing data of node using the Future object should only wait until
/// the requested node is evaluated not the entire graph
TEST(GraphExecutionModel, future_get)
{
    constexpr double EXPECTED_VALUE_A = 42.0;
    constexpr double EXPECTED_VALUE_D = 2 * EXPECTED_VALUE_A;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.evaluateGraph();

    gtTrace() << "Waiting for node A...";
    // Here the future should only wait for node A, thus A should be evaluated,
    // but node C and D (maybe node B) should still be evaluating
    auto dataA = future.get(A_uuid, PortType::Out, PortIndex(0), maxTimeout).as<DoubleData>();
    ASSERT_TRUE(dataA);
    EXPECT_DOUBLE_EQ(dataA->value(), EXPECTED_VALUE_A);
    EXPECT_TRUE(model.isNodeEvaluated(A_uuid));

    EXPECT_FALSE(model.isNodeEvaluated(C_uuid));
    EXPECT_FALSE(model.isNodeEvaluated(D_uuid));

    // a subsequent call should not cause a second wait
    dataA = future.get(A_uuid, PortType::Out, PortIndex(0), directTimeout).as<DoubleData>();
    ASSERT_TRUE(dataA);
    EXPECT_DOUBLE_EQ(dataA->value(), EXPECTED_VALUE_A);

    debug(model);

    gtTrace() << "Waiting for node D...";
    // Here the future should wait for node D and by extension for all other
    //nodes, thus A, B, C, and D should be evaluated
    auto dataD = future.get(D_uuid, PortType::In, PortIndex(0), maxTimeout).as<DoubleData>();
    ASSERT_TRUE(dataD);
    EXPECT_DOUBLE_EQ(dataD->value(), EXPECTED_VALUE_D);
    EXPECT_TRUE(model.isNodeEvaluated(A_uuid));
    EXPECT_TRUE(model.isNodeEvaluated(B_uuid));
    EXPECT_TRUE(model.isNodeEvaluated(C_uuid));
    EXPECT_TRUE(model.isNodeEvaluated(D_uuid));

    // a subsequent call should not cause a second wait
    dataD = future.get(D_uuid, PortType::In, PortIndex(0), directTimeout).as<DoubleData>();
    ASSERT_TRUE(dataD);
    EXPECT_DOUBLE_EQ(dataD->value(), EXPECTED_VALUE_D);

    debug(model);
}

/// The future class allows the creation of an async callback function once the
/// targets nodes in the future have finished evaluation
TEST(GraphExecutionModel, future_then)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    bool functionCalled = false;

    {
        gtTrace() << "Evaluate...";
        // wait for a specific node...
        auto future = model.evaluateNode(A_uuid);
        // .. or multiple nodes...
        future.join(model.evaluateNode(B_uuid));
        // ... or the entire graph
        future.join(model.evaluateGraph());

        // async callback once all targets have evaluated
        future.then([&functionCalled](bool success){
            ASSERT_TRUE(success);
            functionCalled = true;
            gtTrace() << "Callback function called!";
        });

        EXPECT_FALSE(model.isGraphEvaluated());
        EXPECT_FALSE(functionCalled);

        // using future here to wait until graph evaluated
        EXPECT_TRUE(future.wait(maxTimeout));
    }

    gtTrace() << "Validate...";
    EXPECT_TRUE(functionCalled);
    EXPECT_TRUE(model.isGraphEvaluated());

    // Callback Function should only evaluate once
    gtTrace() << "Invalidate...";
    functionCalled = false;

    model.invalidateNode(A_uuid);
    EXPECT_FALSE(model.isGraphEvaluated());

    {
        auto future = model.evaluateGraph();
        EXPECT_TRUE(future.wait(maxTimeout));
    }

    EXPECT_FALSE(functionCalled);
    EXPECT_TRUE(model.isGraphEvaluated());

    debug(model);

    functionCalled = false;

    // should be triggered if evaluation failed directly
    {
        gtTrace() << "Evaluate invalid node...";

        auto future = model.evaluateNode(E_uuid);
        future.then([&functionCalled](bool success){
            EXPECT_FALSE(success);
            functionCalled = true;
            gtTrace() << "Callback function called!";
        });

        EXPECT_TRUE(functionCalled);
    }

    debug(model);
}

/// The callback function of a future class is called once a timeout triggers.
TEST(GraphExecutionModel, future_then_with_timeout)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    bool functionCalled = false;

    {
        gtTrace() << "Evaluate...";

        auto future = model.evaluateNode(D_uuid);
        future.then([&functionCalled](bool success){
            EXPECT_FALSE(success);
            functionCalled = true;
            gtTrace() << "Callback function called!";
        }, directTimeout);

        EXPECT_FALSE(model.isGraphEvaluated());
        EXPECT_FALSE(functionCalled);

        // using future here to wait until graph evaluated
        EXPECT_TRUE(future.wait(maxTimeout));
    }

    gtTrace() << "Validate...";
    EXPECT_TRUE(functionCalled);
    EXPECT_TRUE(model.isGraphEvaluated());
}
