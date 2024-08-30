/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "test_helper.h"

#include "intelli/future.h"
#include "intelli/memory.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>
#include <gt_eventloop.h>

/* TODO:
 * - Evaluate multiple exclusive nodes
 * - Check evaluation of paused nodes
 * - Waiting for node that is deleted
 */

using namespace intelli;

constexpr auto maxTimeout = std::chrono::seconds(1);
constexpr auto directTimeout = std::chrono::seconds(0);

/// Evaluating a node should evaluate all of its dependencies. In this case
/// we only evalutate the first node, which has no dependencies. Thus, only
/// the first node should be valid, and the successor nodes should have the result
/// data as inputs only.
TEST(GraphExecutionModel, linear_graph__evaluate_first_node)
{
    constexpr double EXPECTED_VALUE = 42.0;

    Graph graph;

    GraphExecutionModel model(graph);

    gtTrace() << "Setup...";
    ASSERT_TRUE(test::buildLinearGraph(graph));

    auto nodeA = graph.findNode(A_id);
    ASSERT_TRUE(nodeA);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(model.evaluateNode(A_uuid).wait(maxTimeout));
    EXPECT_TRUE(model.isNodeEvaluated(A_uuid));

    debug(model);

    gtTrace() << "Validate results...";
    /// RESULTS
    auto dataA = model.nodeData(A_uuid, PortType::Out, PortIndex(0)).as<DoubleData>();
    ASSERT_TRUE(dataA);
    EXPECT_DOUBLE_EQ(dataA->value(), EXPECTED_VALUE);

    // only node A should be evaluated and valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid}, NodeEvalState::Valid));
    // all other nodes are still outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // only port 0 is connected
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, B_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE));
    // all other ports are still outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, B_uuid, {PortId(1), PortId(2)}, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Invalidate...";
    emit nodeA->triggerNodeEvaluation();

    EXPECT_FALSE(model.isNodeEvaluated(A_uuid));

    // now all nodes are outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // both B and A still have their data value associated, but its now outdated
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, B_uuid, {PortId(0)}, PortDataState::Outdated, EXPECTED_VALUE));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, A_uuid, {PortId(0)}, PortDataState::Outdated, EXPECTED_VALUE));
    // all other nodes are still outdated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, PortDataState::Outdated));

    debug(model);
}

/// Evaluating a node should evaluate all of its dependencies. In this case
/// the last has all other nodes as dependencies, thus the entire graph should
/// be evaluated and valid.
TEST(GraphExecutionModel, linear_graph__evaluate_last_node_and_dependencies)
{
    constexpr double EXPECTED_VALUE = 84.0;

    Graph graph;

    gtTrace() << "Setup...";
    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    auto nodeD = graph.findNode(D_id);
    ASSERT_TRUE(nodeD);

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(model.evaluateNode(D_uuid).wait(maxTimeout));
    EXPECT_TRUE(model.isNodeEvaluated(D_uuid));
    EXPECT_TRUE(model.isGraphEvaluated());

    debug(model);

    gtTrace() << "Validate results...";
    auto dataD = model.nodeData(D_uuid, PortType::In, PortIndex(0)).as<DoubleData>();
    ASSERT_TRUE(dataD);
    EXPECT_EQ(dataD->value(), EXPECTED_VALUE);

    // node D and all other dependencies must have been evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));

    gtTrace() << "Invalidate...";
    emit nodeD->triggerNodeEvaluation();

    EXPECT_FALSE(model.isNodeEvaluated(D_uuid));

    // only node D has been invalidated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {D_uuid}, NodeEvalState::Outdated));
    // all other nodes are still valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid}, NodeEvalState::Valid));

    debug(model);
}

TEST(GraphExecutionModel, linear_graph__auto_evaluate_graph)
{
    constexpr double EXPECTED_VALUE = 84.0;

    Graph graph;

    gtTrace() << "Setup...";
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

/// A basic graph which contains a group node (subgraph) is tested. However,
/// this group node is setup in such a way, that the ports of the input provider
/// are directly connected to the output ports of the output provider. Thus any
/// input data of the group node should be forwarded to the output.
TEST(GraphExecutionModel, graph_with_forwarding_group__evaluate_group_node)
{
    constexpr double EXPECTED_VALUE_IN1 = 26.0;
    constexpr double EXPECTED_VALUE_IN2 = 8.0;
    constexpr double EXPECTED_VALUE_OUT1 = EXPECTED_VALUE_IN1;
    constexpr double EXPECTED_VALUE_OUT2 = EXPECTED_VALUE_IN2;

    Graph graph;

    GraphExecutionModel model(graph);

    gtTrace() << "Setup...";
    ASSERT_TRUE(test::buildGraphWithForwardingGroup(graph));

    auto* group = qobject_cast<Graph*>(graph.findNode(group_id));
    ASSERT_TRUE(group);

    debug(graph);
    debug(model);

    ASSERT_FALSE(A_uuid.isEmpty());

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

    for (auto& x : graph.globalConnectionModel().find(B_uuid)->successors)
    {
        gtDebug() << "SUCCESSOR" << x.node << x.port;
    }

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(model.evaluateNode(group_uuid).wait(maxTimeout));

    debug(model);

    gtTrace() << "Validate results...";
    PortId group_input1 = group->portId(PortType::In, PortIndex(0));
    PortId group_input2 = group->portId(PortType::In, PortIndex(1));
    PortId group_output1 = group->portId(PortType::Out, PortIndex(0));
    PortId group_output2 = group->portId(PortType::Out, PortIndex(1));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input1}, PortDataState::Valid, EXPECTED_VALUE_IN1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input2}, PortDataState::Valid, EXPECTED_VALUE_IN2));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_output1}, PortDataState::Valid, EXPECTED_VALUE_OUT1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_output2}, PortDataState::Valid, EXPECTED_VALUE_OUT2));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, A_uuid, PortDataState::Valid, EXPECTED_VALUE_IN1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, B_uuid, PortDataState::Valid, EXPECTED_VALUE_IN2));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input1}, PortDataState::Valid, EXPECTED_VALUE_IN1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {group_input2}, PortDataState::Valid, EXPECTED_VALUE_IN2));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output1}, PortDataState::Valid, EXPECTED_VALUE_OUT1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {group_output2}, PortDataState::Valid, EXPECTED_VALUE_OUT2));

    // node E should also have recieved the input data from B
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, E_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_IN2));

    // node D should also have rcieved the input data from A and B
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_OUT1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, D_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_IN2));

    // all other nodes should still be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {D_uuid, E_uuid}, NodeEvalState::Outdated));

    debug(model);

    gtTrace() << "Invalidate...";
    emit group->triggerNodeEvaluation();

    // only input nodes to group are still valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid}, NodeEvalState::Valid));
    // all other nodes are outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {C_uuid, D_uuid, E_uuid}, NodeEvalState::Outdated));
}

TEST(GraphExecutionModel, graph_with_forwarding_group__evaluate_graph)
{
    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B = 8.0;
    constexpr double EXPECTED_VALUE_D = 34.0;

    Graph graph;

    GraphExecutionModel model(graph);

    gtTrace() << "Setup...";
    ASSERT_TRUE(test::buildGraphWithForwardingGroup(graph));

    auto* group = qobject_cast<Graph*>(graph.findNode(group_id));
    ASSERT_TRUE(group);
    auto* A = graph.findNode(A_id);
    ASSERT_TRUE(A);
    auto* B = graph.findNode(B_id);
    ASSERT_TRUE(B);

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
    EXPECT_TRUE(model.evaluateGraph().wait(maxTimeout));

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

    debug(model);

    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {E_uuid, B_uuid}, NodeEvalState::Valid));
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, group_uuid, group_input_uuid, group_output_uuid, D_uuid}, NodeEvalState::Outdated));

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
}

TEST(GraphExecutionModel, graph_with_forwarding_group__auto_evaluate_graph)
{
    constexpr double EXPECTED_VALUE_A = 26.0;
    constexpr double EXPECTED_VALUE_B = 8.0;
    constexpr double EXPECTED_VALUE_D = 34.0;

    Graph graph;

    GraphExecutionModel model(graph);

    gtTrace() << "Setup...";
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

TEST(GraphExecutionModel, graph_with_basic_group__auto_evaluate_graph)
{
    Graph graph;

    test::buildGraphWithGroup(graph);

    auto* subGraph = graph.findNode(group_id);
    ASSERT_TRUE(subGraph);

    GraphExecutionModel model(graph);

    debug(model);

    gtDebug() << "Evaluate...";
    EXPECT_TRUE(model.autoEvaluateGraph().wait(maxTimeout));

    gtDebug() << "Scheudling deletion of subgraph...";
    subGraph->deleteLater();

    gtDebug() << "Starting local event loop...";
    GtEventLoop loop(directTimeout);
    loop.exec();

    debug(model);
}

TEST(GraphExecutionModel, auto_evaluate_graph_and_remove_connections)
{
    constexpr double EXPECTED_VALUE_1 = 84.0;
    constexpr double EXPECTED_VALUE_2 = 42.0;

    Graph graph;

    gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";

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

    gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";
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

TEST(GraphExecutionModel, stop_auto_evaluating_graph)
{
    Graph graph;

    gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";
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

#if 0
TEST(GraphExecutionModel, auto_evaluate_subgraph_without_connection_between_input_and_output_provider)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    bool success = true;
    success &= graph.deleteConnection(ConnectionId{B_id, PortId(0), E_id, PortId(0)});
    success &= graph.deleteConnection(ConnectionId{B_id, PortId(0), D_id, PortId(1)});
    success &= graph.deleteNode(E_id);
    ASSERT_TRUE(success);

    debug(graph);

    auto subGraphs = graph.graphNodes();
    ASSERT_EQ(subGraphs.size(), 1);

    Graph& subGraph = *subGraphs.at(0);
    ASSERT_TRUE(&subGraph);

    success &= subGraph.deleteConnection(ConnectionId{group_input_id, PortId(0), group_B_id, PortId(1)});
    success &= subGraph.deleteConnection(ConnectionId{group_input_id, PortId(1), group_C_id, PortId(1)});
    ASSERT_TRUE(success);

    debug(subGraph);

    GraphExecutionModel model(graph);

    EXPECT_FALSE(subGraph.executionModel());

    auto& submodel = *subGraph.makeExecutionModel();
    ASSERT_TRUE(&submodel);

    EXPECT_FALSE(model.isEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(D_id));

    EXPECT_FALSE(submodel.isEvaluated());
    EXPECT_FALSE(submodel.isNodeEvaluated(group_D_id));
    EXPECT_FALSE(submodel.isNodeEvaluated(group_output_id));

    auto future = model.autoEvaluate();
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_FALSE(model.isNodeEvaluated(group_D_id));
    EXPECT_TRUE(submodel.isNodeEvaluated(group_output_id));

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).as<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);

    gtDebug() << "";

    model.reset();

    EXPECT_FALSE(model.isEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(D_id));

    EXPECT_FALSE(submodel.isEvaluated());
    EXPECT_FALSE(submodel.isNodeEvaluated(group_D_id));
    EXPECT_FALSE(submodel.isNodeEvaluated(group_output_id));

    future = model.autoEvaluate();
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_FALSE(submodel.isNodeEvaluated(group_D_id));
    EXPECT_TRUE(submodel.isNodeEvaluated(group_output_id));

    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).as<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);
}
#endif

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

TEST(GraphExecutionModel, do_not_evaluate_cyclic_graphs)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {
        auto& value1   = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& value2   = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));

        auto& add1 = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));
        auto& add2 = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));

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

    EXPECT_FALSE(model.autoEvaluate().wait(std::chrono::seconds(1)));
    EXPECT_FALSE(model.isEvaluated());

    EXPECT_FALSE(model.evaluateGraph().wait(std::chrono::seconds(1)));
    EXPECT_FALSE(model.isEvaluated());

    EXPECT_FALSE(model.evaluateNode(E_id).wait(std::chrono::seconds(1)));

    EXPECT_FALSE(model.isEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(E_id));
}
#endif

/// Destroying the graph exec model while its running should not cause any harm
TEST(GraphExecutionModel, destroy_while_running)
{
    Graph graph;

    {
        GraphExecutionModel model(graph);

        gtTrace() << "Setup...";
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

    gtTrace() << "Setup...";
    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    gtTrace() << "Evaluate...";
    auto future = model.evaluateGraph();

    gtTrace() << "Waiting for node A...";
    // Here the future should only wait for node A, thus A should be evaluated,
    // but node B, C, and D should still be evaluating
    auto dataA = future.get(A_uuid, PortType::Out, PortIndex(0), maxTimeout).as<DoubleData>();
    ASSERT_TRUE(dataA);
    EXPECT_DOUBLE_EQ(dataA->value(), EXPECTED_VALUE_A);
    EXPECT_TRUE(model.isNodeEvaluated(A_uuid));

    EXPECT_FALSE(model.isNodeEvaluated(B_uuid));
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

/// The future allows the creation of an async call back function once the
/// targets nodes in the future have finished evaluation
TEST(GraphExecutionModel, future_then)
{
    Graph graph;

    GraphExecutionModel model(graph);

    gtTrace() << "Setup...";
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
}
