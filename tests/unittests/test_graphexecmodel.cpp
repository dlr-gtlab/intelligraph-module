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

#include "intelli/future.h"
#include "intelli/memory.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"

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
            A_uuid, B_uuid, D_uuid, E_uuid,
            group_uuid, group_input_uuid, group_output_uuid,
            group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid
        }, NodeEvalState::Valid));

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
        // only succesor are evaluated
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
    Node& A1 = builder.addNode("intelli::NumberSourceNode", "A1_uuid").setCaption("A1");
    Node& A2 = builder.addNode("intelli::NumberSourceNode", "A2_uuid").setCaption("A2");

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

    gtTrace() << "Scheudling callback function...";

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

    gtTrace() << "Scheudling callback function...";

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

#if 0
TEST(GraphExecutionModel, linear_graph__auto_evaluate_graph)
{
    constexpr double EXPECTED_VALUE = 84.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    auto nodeB = graph.findNode(B_id);
    ASSERT_TRUE(nodeB);

    debug(graph);
    debug(model);

    /// SETUP
    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortType::In, PortIndex(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Invalidate...";

    emit nodeB->triggerNodeEvaluation();

    EXPECT_FALSE(model.isNodeEvaluated(B_uuid));
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortType::In, PortIndex(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, graph_with_forwarding_group__auto_evaluate_graph)
{
    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B = 8.0;
    constexpr double EXPECTED_VALUE_D = 34.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithForwardingGroup(graph));

    auto* group = qobject_cast<Graph*>(graph.findNode(group_id));
    ASSERT_TRUE(group);
    auto* A = graph.findNode(A_id);
    ASSERT_TRUE(A);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid, group_input_uuid, group_output_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model,
        {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid, group_input_uuid, group_output_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";
    // all nodes should be evaluated and valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid, group_input_uuid, group_output_uuid},
        NodeEvalState::Valid));

    // A was evaluated and has the expected value
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, A_uuid, PortDataState::Valid, EXPECTED_VALUE_A));
    // B is connected to E, both should share the same value
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {B_uuid, E_uuid}, PortDataState::Valid, EXPECTED_VALUE_B));

    PortId group_input1 = group->portId(PortType::In, PortIndex(0));
    PortId group_input2 = group->portId(PortType::In, PortIndex(1));
    PortId group_output1 = group->portId(PortType::Out, PortIndex(0));
    PortId group_output2 = group->portId(PortType::Out, PortIndex(1));

    // Group inputs and outputs forward value from A and B
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input1, group_output1}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input2, group_output2}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input1}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input2}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output1}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output2}, PortDataState::Valid, EXPECTED_VALUE_B));

    // D was evaluated correctly
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(2)}, PortDataState::Valid, EXPECTED_VALUE_D));

    gtTrace() << "Invalidate...";
    emit A->triggerNodeEvaluation();

    // node A is already evaluating
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Evaluating));
    // unaffected nodes are still valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {E_uuid, B_uuid}, NodeEvalState::Valid));
    // all other are now outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {group_uuid, group_input_uuid, group_output_uuid, D_uuid}, NodeEvalState::Outdated));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {B_uuid, E_uuid}, PortDataState::Valid));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, A_uuid, PortDataState::Outdated));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input1, group_output1}, PortDataState::Outdated, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input2}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_output2}, PortDataState::Outdated, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input1}, PortDataState::Outdated, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input2}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output1}, PortDataState::Outdated, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output2}, PortDataState::Valid, EXPECTED_VALUE_B));

    // D was evaluated correctly
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(0)}, PortDataState::Outdated, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(2)}, PortDataState::Outdated, EXPECTED_VALUE_D));

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(future.wait(maxTimeout));

    gtTrace() << "Validate results...";
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid, group_input_uuid, group_output_uuid},
        NodeEvalState::Valid));

    debug(model);
}

TEST(GraphExecutionModel, auto_evaluate_graph_and_remove_connections)
{
    constexpr double EXPECTED_VALUE_1 = 84.0;
    constexpr double EXPECTED_VALUE_2 = 42.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_1);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Remove connection...";
    auto conId = graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0));
    ASSERT_TRUE(graph.deleteConnection(conId));
    ASSERT_FALSE(graph.findConnection(conId));

    {
        // All nodes before the change are still valid
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid}, NodeEvalState::Valid));

        // Node C should be re-evaluating
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {C_uuid}, NodeEvalState::Evaluating));

        // All other nodes should be outdated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {D_uuid}, NodeEvalState::Outdated));

        EXPECT_TRUE(future.wait(maxTimeout));
        EXPECT_TRUE(model.isGraphEvaluated());
    }

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_2);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, auto_evaluate_graph_and_remove_node)
{
    constexpr double EXPECTED_VALUE_1 = 84.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_1);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Remove node...";
    ASSERT_TRUE(graph.deleteNode(A_id));

    {
        // Deleted node can no longer be found -> invalid
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid}, NodeEvalState::Invalid));

        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {B_uuid}, NodeEvalState::Evaluating));

        // All other nodes should be outdated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {C_uuid, D_uuid}, NodeEvalState::Outdated));
    }

    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        EXPECT_EQ(dataD, nullptr);

        // all nodes must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, auto_evaluate_graph_and_append_connection_only)
{
    constexpr double EXPECTED_VALUE_1 = 42.0;
    constexpr double EXPECTED_VALUE_2 = 84.0;

    Graph graph;

    ASSERT_TRUE(test::buildLinearGraph(graph));

    auto conId = graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0));
    ASSERT_TRUE(graph.deleteConnection(conId));

    GraphExecutionModel model(graph);

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_1);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Appending connection...";
    GraphBuilder builder(graph);
    builder.connect(B_id, PortIndex(0), C_id, PortIndex(0));

    {
        // All nodes before the change are still valid
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid}, NodeEvalState::Valid));

        // Node C should be re-evaluating
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {C_uuid}, NodeEvalState::Evaluating));

        // All other nodes should be outdated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {D_uuid}, NodeEvalState::Outdated));
    }

    gtTrace() << "Awaiting results...";

    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_2);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, auto_evaluate_graph_and_append_node_and_connection)
{
    constexpr double EXPECTED_VALUE_1 = 84.0;
    constexpr double EXPECTED_VALUE_2 = 54.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_1);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Modifying graph...";

    {
        auto change = graph.modify();
        Q_UNUSED(change);

        gtTrace() << "Remove connection...";
        auto conId = graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0));
        ASSERT_TRUE(graph.deleteConnection(conId));
        ASSERT_FALSE(graph.findConnection(conId));

        gtTrace() << "Append node E...";
        GraphBuilder builder(graph);
        auto& E = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), E_uuid)
                      .setCaption("E");

        // E is not connected -> auto evaluate
        EXPECT_TRUE(model.isAutoEvaluatingNode(E_uuid));

        gtTrace() << "Append connection...";
        builder.connect(E_id, PortIndex(0), C_id, PortIndex(0));

        // E is now connected -> do not auto evaluate anymore
        EXPECT_FALSE(model.isAutoEvaluatingNode(E_uuid));

        gtTrace() << "Set value of E...";
        setNodeProperty(E, QStringLiteral("value"), 12);

        debug(graph);
        debug(model);
    }

    gtTrace() << "Awaiting results...";

    {
        // All nodes before the change are still valid
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid}, NodeEvalState::Valid));

        // Node C should be re-evaluating
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {E_uuid}, NodeEvalState::Evaluating));

        // All other nodes should be outdated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {C_uuid, D_uuid}, NodeEvalState::Outdated));
    }

    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_2);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid, E_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, auto_evaluate_graph_triggered_by_set_node_data)
{
    constexpr double EXPECTED_VALUE_1 = 84.0;
    constexpr double EXPECTED_VALUE_2 = 90.0;

    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();
    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_1);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }

    gtTrace() << "Invalidate...";

    model.setNodeData(B_uuid, PortId(1), std::make_shared<DoubleData>(3));

    EXPECT_FALSE(model.isNodeEvaluated(B_uuid));
    EXPECT_FALSE(model.isGraphEvaluated());

    EXPECT_TRUE(future.wait(maxTimeout));
    EXPECT_TRUE(model.isGraphEvaluated());

    gtTrace() << "Validate results...";

    {
        auto dataD = model.nodeData(D_uuid, PortId(0)).as<DoubleData>();
        ASSERT_TRUE(dataD);
        EXPECT_EQ(dataD->value(), EXPECTED_VALUE_2);

        // node D and all other dependencies must have been evaluated
        EXPECT_TRUE(test::compareNodeEvalState(
            graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));
    }
}

TEST(GraphExecutionModel, auto_evaluate_subgraph_only)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    GraphExecutionModel model(graph);

    auto const& subgraphs = graph.graphNodes();
    ASSERT_EQ(subgraphs.size(), 1);
    auto& subgraph = *subgraphs.first();

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate subgraph...";
    auto future = model.autoEvaluateGraph(subgraph);

    EXPECT_TRUE(model.isAutoEvaluatingGraph(subgraph));
    EXPECT_FALSE(model.isAutoEvaluatingGraph(graph));
    EXPECT_FALSE(model.isGraphEvaluated(subgraph));
    EXPECT_FALSE(model.isGraphEvaluated(graph));

    EXPECT_TRUE(future.wait(maxTimeout));

    gtTrace() << "Validate results...";

    debug(model);

    // Dependencies of subgraph (and the group node itself) were evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid}, NodeEvalState::Valid));

    // all other nodes were not triggered
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {D_uuid, E_uuid}, NodeEvalState::Outdated));

    // all nodes in the graph were evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {
            group_A_uuid, group_B_uuid, group_C_uuid,
            group_D_uuid, group_input_uuid, group_output_uuid
        }, NodeEvalState::Valid));

    EXPECT_TRUE(model.isAutoEvaluatingGraph(subgraph));
    EXPECT_FALSE(model.isAutoEvaluatingGraph(graph));
    EXPECT_TRUE(model.isGraphEvaluated(subgraph));
    EXPECT_FALSE(model.isGraphEvaluated(graph));
}

TEST(GraphExecutionModel, auto_evaluate_subgraph_without_connection_between_input_and_output_provider)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    auto subgraphs = graph.graphNodes();
    ASSERT_EQ(subgraphs.size(), 1);

    Graph& subgraph = *subgraphs.at(0);
    ASSERT_TRUE(&subgraph);

    // remove connection between top-level group inputs and output nodes
    ASSERT_TRUE(graph.deleteConnection(ConnectionId{B_id, PortId(0), E_id, PortId(0)}));
    ASSERT_TRUE(graph.deleteConnection(ConnectionId{B_id, PortId(0), D_id, PortId(1)}));
    ASSERT_TRUE(graph.deleteNode(E_id));

    // delete connections with input provider
    ASSERT_TRUE(subgraph.deleteConnection(subgraph.connectionId(group_input_id, PortIndex(0), group_B_id, PortIndex(1))));
    ASSERT_TRUE(subgraph.deleteConnection(subgraph.connectionId(group_input_id, PortIndex(1), group_C_id, PortIndex(1))));

    GraphExecutionModel model(graph);

    debug(graph);

    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid, D_uuid,
         group_uuid, group_input_uuid, group_output_uuid,
         group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid},
        NodeEvalState::Outdated));

    EXPECT_FALSE(model.isGraphEvaluated());
    EXPECT_FALSE(model.isGraphEvaluated(subgraph));
    EXPECT_FALSE(model.isNodeEvaluated(group_D_uuid));
    EXPECT_FALSE(model.isNodeEvaluated(D_uuid));

    gtTrace() << "Evaluate...";

    auto future = model.autoEvaluateGraph(subgraph);
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    debug(model);

    gtTrace() << "Validate results...";

    // all nodes that are not required to evaluate subgraph are still outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {D_uuid}, NodeEvalState::Outdated));
    // all other nodes are valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model,
        {A_uuid, B_uuid,
         group_uuid, group_input_uuid, group_output_uuid,
         group_A_uuid, group_B_uuid, group_C_uuid, group_D_uuid},
        NodeEvalState::Valid));

    auto data = model.nodeData(group_uuid, PortType::Out, PortIndex(0)).as<DoubleData>();
    ASSERT_TRUE(data);
    EXPECT_EQ(data->value(), 8);
}
#endif

TEST(GraphExecutionModel, evaluation_of_exclusive_nodes)
{
    std::chrono::seconds maxTimeout(4);

    Graph graph;

    GraphBuilder builder(graph);

    Node& S = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), "S_UUID")
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
    C.setNodeEvalMode(NodeEvalMode::ExclusiveDetached);

    GraphExecutionModel model(graph);

    int nodeEvaluations = 0;
    bool callbackFunctionCalled = false;

    QObject::connect(&model, &GraphExecutionModel::nodeEvalStateChanged,
                     &model, [&](NodeUuid const& nodeUuid){
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
    });

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

TEST(GraphExecutionModel, evaluation_of_exclusive_nodes_in_multiple_graphs)
{
    std::chrono::seconds maxTimeout(10);

    QVector<QStringList> nodeMap;

    auto setupGraph = [&](Graph& g){
        GraphBuilder builder(g);

        Node& S = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), g.caption() +  "S_UUID")
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

    auto onNodeEvalStateChanged = [&](NodeUuid const& uuid){
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

    QObject::connect(&model1, &GraphExecutionModel::nodeEvalStateChanged,
                     &model1, onNodeEvalStateChanged);
    QObject::connect(&model2, &GraphExecutionModel::nodeEvalStateChanged,
                     &model2, onNodeEvalStateChanged);

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

#if 0
TEST(GraphExecutionModel, do_not_auto_evaluate_inactive_nodes)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    debug(graph);

    // auto evaluate

    Node* A = graph.findNode(A_id);
    Node* B = graph.findNode(B_id);
    Node* C = graph.findNode(C_id);
    Node* D = graph.findNode(D_id);
    Node* E = graph.findNode(E_id);

    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    ASSERT_TRUE(E);

    // make node C inactive
    C->setActive(false);

    GraphExecutionModel model(graph);

    EXPECT_FALSE(model.isEvaluated());

    EXPECT_FALSE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_FALSE(model.isEvaluated());

    // node C an subsequent nodes were not evaluated
    EXPECT_FALSE(model.isNodeEvaluated(C_id));
//    EXPECT_TRUE(C->nodeFlags() & NodeFlag::RequiresEvaluation);
    EXPECT_FALSE(C->nodeFlags() & NodeFlag::Evaluating);

    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(C_data.state, PortDataState::Outdated);
    EXPECT_EQ(C_data.ptr, nullptr);

    EXPECT_FALSE(model.isNodeEvaluated(D_id));
//    EXPECT_TRUE(D->nodeFlags() & NodeFlag::RequiresEvaluation);
    EXPECT_FALSE(D->nodeFlags() & NodeFlag::Evaluating);

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(D_data.state, PortDataState::Outdated);
    EXPECT_EQ(D_data.ptr, nullptr);

    // Node A, B, E were evaluated
    EXPECT_FALSE(A->nodeFlags() & NodeFlag::Evaluating);
    EXPECT_FALSE(B->nodeFlags() & NodeFlag::Evaluating);
    EXPECT_FALSE(E->nodeFlags() & NodeFlag::Evaluating);

    EXPECT_TRUE(model.isNodeEvaluated(A_id));
    EXPECT_TRUE(model.isNodeEvaluated(B_id));
    EXPECT_TRUE(model.isNodeEvaluated(E_id));

    auto A_data = model.nodeData(A_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(A_data.state, PortDataState::Valid);
    EXPECT_NE(A_data.ptr, nullptr);

    auto B_data = model.nodeData(B_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(B_data.state, PortDataState::Valid);
    EXPECT_NE(B_data.ptr, nullptr);

    auto E_data = model.nodeData(E_id, PortType::In, PortIndex(0));
    EXPECT_EQ(E_data.state, PortDataState::Valid);
    EXPECT_NE(E_data.ptr, nullptr);

    // set C as active -> the whole graph should be evaluated
    C->setActive();

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());

    EXPECT_FALSE(C->nodeFlags() & NodeFlag::Evaluating);
    EXPECT_FALSE(D->nodeFlags() & NodeFlag::Evaluating);

    EXPECT_TRUE(model.isNodeEvaluated(C_id));
    EXPECT_TRUE(model.isNodeEvaluated(D_id));
}
#endif

TEST(GraphExecutionModel, evaluation_of_cyclic_graph)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {
        auto& value1   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), A_uuid).setCaption(QStringLiteral("A"));
        auto& value2   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), B_uuid).setCaption(QStringLiteral("B"));

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
}

#if 0
TEST(GraphExecutionModel, stop_auto_evaluating_graph)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateGraph();

    // abort execution once node A is finished
    model.evaluateNode(A_uuid).then([&model](bool success){
        gtTrace() << "Stopping auto evaluation...";
        model.stopAutoEvaluatingGraph();
    });

    EXPECT_TRUE(model.isAutoEvaluatingGraph());

    // A is not yet evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Evaluating));

    // Waiting for all nodes fails because the graph is stopped inbetween
    EXPECT_FALSE(future.wait(maxTimeout));

    EXPECT_FALSE(model.isAutoEvaluatingGraph());
    EXPECT_FALSE(model.isGraphEvaluated());

    // Node A is evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Valid));

    // All other nodes still have to be evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));
}

TEST(GraphExecutionModel, stop_auto_evaluating_node)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.autoEvaluateNode(C_uuid);

    // abort execution once node A is finished
    model.evaluateNode(A_uuid).then([&model](bool success){
        gtTrace() << "Stopping auto evaluation...";
        model.stopAutoEvaluatingNode(C_uuid);
    });

    EXPECT_TRUE(model.isAutoEvaluatingNode(C_uuid));

    // A is not yet evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Evaluating));

    // Waiting for all nodes fails because the graph is stopped inbetween
    EXPECT_FALSE(future.wait(maxTimeout));

    EXPECT_FALSE(model.isAutoEvaluatingNode(C_uuid));
    EXPECT_FALSE(model.isGraphEvaluated());

    // Node A is evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Valid));

    // All other nodes still have to be evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));
}
#endif

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
        auto graph = make_volatile<Graph>();

        ASSERT_TRUE(test::buildGraphWithGroup(*graph));

        auto model = make_volatile<GraphExecutionModel>(*graph);
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
