/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "test_helper.h"

#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/dynamicnode.h"

#include "gt_eventloop.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;

constexpr auto timeout = std::chrono::seconds(1);

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
    EXPECT_TRUE(model.evaluateNode(A_uuid).wait(timeout));

    debug(model);

    gtTrace() << "Validate results...";
    /// RESULTS
    auto dataA = model.nodeData(A_uuid, PortId(0)).value<DoubleData>();
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
    EXPECT_TRUE(model.evaluateNode(D_uuid).wait(timeout));

    debug(model);

    gtTrace() << "Validate results...";
    auto dataD = model.nodeData(D_uuid, PortId(0)).value<DoubleData>();
    ASSERT_TRUE(dataD);
    EXPECT_EQ(dataD->value(), EXPECTED_VALUE);

    // node D and all other dependencies must have been evaluated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Valid));

    gtTrace() << "Invalidate...";
    emit nodeD->triggerNodeEvaluation();

    // only node D has been invalidated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {D_uuid}, NodeEvalState::Outdated));
    // all other nodes are still valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid}, NodeEvalState::Valid));

    debug(model);
}

#if 0
TEST(GraphExecutionModel, lineaer_graph__auto_evaluate_graph)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);
    debug(model);

    /// SETUP
    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, C_uuid, D_uuid}, PortDataState::Outdated, {nullptr}));

    /// EVALUATION START
    //        EXPECT_TRUE(model.autoEvaluateGraph();

    // let the exection finish
    GtEventLoop eventLoop{std::chrono::seconds{1}};
    eventLoop.exec();
}
#endif

/// A basic graph which contains a group node (subgraph). This group node
/// is however setup in such a way, that the ports of the input provider are
/// directly connected to the output ports of the output provider. Thus any
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

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid,
         group_input_uuid, group_output_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid,
         group_input_uuid, group_output_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(model.evaluateNode(group_uuid).wait(timeout));

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
        graph, model, group_input_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_IN1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_IN2));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_OUT1));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_OUT2));

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

    debug(graph);
    debug(model);

    // all nodes should be outdated
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid,
         group_input_uuid, group_output_uuid},
        NodeEvalState::Outdated));

    // all ports should be outdated and have no data associated
    EXPECT_TRUE(test::comparePortData(
        graph, model, {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid,
         group_input_uuid, group_output_uuid},
        PortDataState::Outdated, {nullptr}));

    gtTrace() << "Evaluate...";
    EXPECT_TRUE(model.evaluateGraph().wait(timeout));

    debug(model);

    gtTrace() << "Validate results...";
    // all nodes should be evaluated and valid
    EXPECT_TRUE(test::compareNodeEvalState(
        graph, model, {A_uuid, B_uuid, group_uuid, D_uuid, E_uuid,
         group_input_uuid, group_output_uuid},
        NodeEvalState::Valid));

    PortId group_input1 = group->portId(PortType::In, PortIndex(0));
    PortId group_input2 = group->portId(PortType::In, PortIndex(1));
    PortId group_output1 = group->portId(PortType::Out, PortIndex(0));
    PortId group_output2 = group->portId(PortType::Out, PortIndex(1));

    // all other nodes are outdated
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {A_uuid}, PortDataState::Valid, EXPECTED_VALUE_A));
    // B is connected to E, both should share the same value
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, {B_uuid, E_uuid}, PortDataState::Valid, EXPECTED_VALUE_B));

    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input1, group_output1}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_uuid, {group_input2, group_output2}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_input_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_B));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {PortId(0)}, PortDataState::Valid, EXPECTED_VALUE_A));
    EXPECT_TRUE(test::comparePortData<double>(
        graph, model, group_output_uuid, {PortId(1)}, PortDataState::Valid, EXPECTED_VALUE_B));
}


#if 0
TEST(GraphExecutionModel, test)
{
    Graph graph;

    GraphExecutionModel model(graph);

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    auto subGraphs = graph.graphNodes();
    ASSERT_EQ(subGraphs.size(), 1);

    Graph* subGraph = subGraphs.at(0);
    ASSERT_TRUE(subGraph);

    gtDebug() << "";
    debug(graph);
    gtDebug() << "";
    debug(model);
    gtDebug() << "";

    auto* input = subGraph->inputNode();
    auto* output = subGraph->outputNode();
    ASSERT_TRUE(input);
    ASSERT_TRUE(output);

    auto* nodeA = graph.findNode(A_id);
    auto* nodeD = graph.findNode(D_id);
    ASSERT_TRUE(nodeA);
    ASSERT_TRUE(nodeD);

    if (0)
    {
        // check `setNodeData` and `nodeData`
        PortId subGraphPortIn0  = subGraph->portId(PortType::In, PortIndex(0));
        PortId subGraphPortIn1  = subGraph->portId(PortType::In, PortIndex(1));
        PortId subGraphPortOut0 = subGraph->portId(PortType::Out, PortIndex(0));

        PortId inputPort0 = input->portId(PortType::Out, PortIndex(0));
        PortId inputPort1 = input->portId(PortType::Out, PortIndex(1));

        PortId outputPort0  = output->portId(PortType::In, PortIndex(0));

        auto data1 = std::make_shared<DoubleData>(42);
        auto data2 = std::make_shared<DoubleData>(12);
        auto data3 = std::make_shared<DoubleData>(33);

        model.setNodeData(subGraph->uuid(), subGraphPortIn0, data1);
        gtDebug() << "";
        model.setNodeData(subGraph->uuid(), subGraphPortIn1, data2);
        gtDebug() << "";
        model.setNodeData(output->uuid(), subGraphPortOut0, data3);

        EXPECT_EQ(model.nodeData(subGraph->uuid(), subGraphPortIn0).ptr,  data1);
        EXPECT_EQ(model.nodeData(subGraph->uuid(), subGraphPortIn1).ptr,  data2);
        EXPECT_EQ(model.nodeData(subGraph->uuid(), subGraphPortOut0).ptr, data3);

        EXPECT_EQ(model.nodeData(input->uuid(), inputPort0).ptr, data1);
        EXPECT_EQ(model.nodeData(input->uuid(), inputPort1).ptr, data2);

        EXPECT_EQ(model.nodeData(output->uuid(), outputPort0).ptr, data3);

        gtDebug() << "";
        debug(model);
        gtDebug() << "";
    }

    model.evaluateNode(nodeD->uuid());
    gtDebug() << "";

    gtDebug() << "### EVENTLOOP START";
    GtEventLoop loop{std::chrono::seconds{1}};
    loop.exec();
    gtDebug() << "### EVENTLOOP END";

    gtDebug() << "";
    debug(model);
    gtDebug() << "";

    if (0)
    {
        input->addOutPort(intelli::typeId<intelli::DoubleData>());
        gtDebug() << "";
        PortId outPort = output->addInPort(intelli::typeId<intelli::DoubleData>());
        gtDebug() << "";
        output->removePort(outPort);

        gtDebug() << "";
        delete subGraph;
        // emitted twice
    }
}
#endif

#if 0
TEST(GraphExecutionModel, evaluate_node)
{
    Graph graph;

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    GraphExecutionModel model(graph);

    auto future = model.evaluateNode(C_id);
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));
    
    EXPECT_TRUE(model.isNodeEvaluated(C_id));

    auto C_data = future.get(PortType::Out, PortIndex(0), std::chrono::seconds(0)).value<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 84);

    // node is already evaluated
    EXPECT_TRUE(model.evaluateNode(C_id).wait(std::chrono::seconds(0)));
}

TEST(GraphExecutionModel, evaluate_node_with_partial_inputs)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("B"));

        builder.connect(A, PortIndex(0), B, PortIndex(0));

        setNodeProperty(A, QStringLiteral("value"), 42);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        ASSERT_NO_THROW(throw e);
    }

    GraphExecutionModel model(graph);

    EXPECT_TRUE(model.evaluateNode(B_id).wait(std::chrono::seconds{1}));

    EXPECT_TRUE(model.isNodeEvaluated(B_id));
    
    auto B_data = model.nodeData(B_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(B_data);
    EXPECT_EQ(B_data->value(), 42);
}

TEST(GraphExecutionModel, evaluate_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildLinearGraph(graph));

    debug(graph);

    GraphExecutionModel model(graph);

    auto future = model.evaluateGraph();

    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());
}

TEST(GraphExecutionModel, auto_evaluate_basic_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    debug(graph);

    // auto evaluate

    GraphExecutionModel model(graph);

    EXPECT_FALSE(model.isEvaluated());

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());
    
    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 42);
    
    auto E_data = model.nodeData(E_id, PortType::In, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(E_data);
    EXPECT_EQ(E_data->value(), 8);

    // disable auto evaluation

    gtDebug() << "";

    model.disableAutoEvaluation();

    EXPECT_TRUE(model.isEvaluated());

    EXPECT_TRUE(model.isNodeEvaluated(D_id));

    model.setNodeData(A_id, PortType::Out, PortIndex(0), std::make_shared<DoubleData>(12));

    // model invalidated
    EXPECT_FALSE(model.isEvaluated());

    EXPECT_FALSE(model.isNodeEvaluated(D_id));

    // old values are still set
    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 42);

    // reenable auto evaluation

    gtDebug() << "";

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(10)));

    // new values is set
    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 28);

    EXPECT_TRUE(model.isEvaluated());

    EXPECT_TRUE(model.isNodeEvaluated(D_id));
}

TEST(GraphExecutionModel, auto_evaluate_graph_with_groups)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    debug(graph);

    auto subGraphs = graph.graphNodes();
    ASSERT_EQ(subGraphs.size(), 1);

    Graph* subGraph = subGraphs.at(0);
    ASSERT_TRUE(subGraph);

    debug(*subGraph);

    // auto evaluate

    GraphExecutionModel model(graph);
    GraphExecutionModel& submodel = *subGraph->makeExecutionModel();
    ASSERT_EQ(&model, graph.executionModel());
    ASSERT_NE(&model, &submodel);

    EXPECT_FALSE(submodel.isEvaluated());

    EXPECT_FALSE(model.isEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(submodel.graph().id()));

    gtDebug() << "Evaluating...";

    auto future = model.autoEvaluate();
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());
    EXPECT_TRUE(model.isNodeEvaluated(submodel.graph().id()));
    
    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 42);
    
    auto D_data = model.nodeData(E_id, PortType::In, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);


    gtDebug() << "";

    setNodeProperty(*graph.findNode(B_id), "value", 10);

    EXPECT_TRUE( model.isNodeEvaluated(A_id));
    EXPECT_FALSE(model.isNodeEvaluated(B_id));
    EXPECT_FALSE(model.isNodeEvaluated(C_id));
    EXPECT_FALSE(model.isNodeEvaluated(D_id));
    EXPECT_FALSE(model.isNodeEvaluated(E_id));

    gtDebug() << "";

    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    gtDebug() << "";

    EXPECT_TRUE(model.isNodeEvaluated(A_id));
    EXPECT_TRUE(model.isNodeEvaluated(B_id));
    EXPECT_TRUE(model.isNodeEvaluated(C_id));
    EXPECT_TRUE(model.isNodeEvaluated(D_id));
    EXPECT_TRUE(model.isNodeEvaluated(E_id));

    C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 44);

    auto E_data = model.nodeData(E_id, PortType::In, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(E_data);
    EXPECT_EQ(E_data->value(), 10);
}

TEST(GraphExecutionModel, auto_evaluate_graph_after_node_deletion)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    debug(graph);

    GraphExecutionModel model(graph);

    auto future = model.autoEvaluate();

    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());

    gtDebug() << "";

    graph.deleteNode(C_id);

    gtDebug() << "";

    EXPECT_FALSE(model.isNodeEvaluated(D_id));

    // model will auto evaluate itself
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isNodeEvaluated(D_id));

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);
}

TEST(GraphExecutionModel, auto_evaluate_subgraph_only)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    debug(graph);

    auto subGraphs = graph.graphNodes();
    ASSERT_EQ(subGraphs.size(), 1);

    Graph* subGraph = subGraphs.at(0);
    ASSERT_TRUE(subGraph);

    debug(*subGraph);

    EXPECT_FALSE(graph.executionModel());
    EXPECT_FALSE(subGraph->executionModel());

    auto& submodel = *subGraph->makeExecutionModel();

    EXPECT_FALSE(submodel.isEvaluated());
    EXPECT_FALSE(submodel.isNodeEvaluated(group_D_id));

    auto future = submodel.evaluateNode(group_D_id);
    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    submodel.debug();

}

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

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
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

    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);
}

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
