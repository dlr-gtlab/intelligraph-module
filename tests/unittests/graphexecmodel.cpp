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

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;

TEST(GraphExecutionModel, test)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    dag::debugGraph(graph.dag());

    GraphExecutionModel model(graph);

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());
}

TEST(GraphExecutionModel, evaluate_until_node)
{
    Graph graph;

    ASSERT_TRUE(test::buildLinearGraph(graph));

    dag::debugGraph(graph.dag());

    GraphExecutionModel model(graph);

    EXPECT_TRUE(model.evaluateNode(C_id).wait(std::chrono::seconds(1)));
    
    EXPECT_TRUE(model.isNodeEvaluated(C_id));

    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).value<DoubleData>();
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

    dag::debugGraph(graph.dag());

    GraphExecutionModel model(graph);

    auto future = model.evaluateGraph();

    EXPECT_TRUE(future.wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());
}

TEST(GraphExecutionModel, auto_evaluate_basic_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_TRUE(isAcyclic(graph));

    dag::debugGraph(graph.dag());

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

    ASSERT_TRUE(test::buildGroupGraph(graph));

    EXPECT_TRUE(isAcyclic(graph));

    dag::debugGraph(graph.dag());

    auto subGraphs = graph.graphNodes();
    ASSERT_EQ(subGraphs.size(), 1);

    Graph* subGraph = subGraphs.at(0);
    ASSERT_TRUE(subGraph);

    dag::debugGraph(subGraph->dag());

    // auto evaluate

    GraphExecutionModel model(graph);
    GraphExecutionModel& submodel = *subGraph->makeExecutionModel();
    ASSERT_EQ(&model, graph.executionModel());
    ASSERT_NE(&model, &submodel);

    EXPECT_FALSE(submodel.isEvaluated());

    EXPECT_FALSE(model.isEvaluated());
    EXPECT_FALSE(model.isNodeEvaluated(submodel.graph().id()));

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_TRUE(submodel.isEvaluated());

    EXPECT_TRUE(model.isEvaluated());
    EXPECT_TRUE(model.isNodeEvaluated(submodel.graph().id()));
    
    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 34);
    
    auto D_data = model.nodeData(E_id, PortType::In, PortIndex(0)).value<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);
}

TEST(GraphExecutionModel, do_not_auto_evaluate_inactive_nodes)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_TRUE(isAcyclic(graph));

    dag::debugGraph(graph.dag());

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
    EXPECT_TRUE(C->nodeFlags() & NodeFlag::RequiresEvaluation);
    EXPECT_FALSE(C->nodeFlags() & NodeFlag::Evaluating);

    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(C_data.state, PortDataState::Outdated);
    EXPECT_EQ(C_data.data, nullptr);

    EXPECT_FALSE(model.isNodeEvaluated(D_id));
    EXPECT_TRUE(D->nodeFlags() & NodeFlag::RequiresEvaluation);
    EXPECT_FALSE(D->nodeFlags() & NodeFlag::Evaluating);

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(D_data.state, PortDataState::Outdated);
    EXPECT_EQ(D_data.data, nullptr);

    // Node A, B, E were evaluated
    EXPECT_FALSE(A->nodeFlags() & (NodeFlag::RequiresEvaluation | NodeFlag::Evaluating));
    EXPECT_FALSE(B->nodeFlags() & (NodeFlag::RequiresEvaluation | NodeFlag::Evaluating));
    EXPECT_FALSE(E->nodeFlags() & (NodeFlag::RequiresEvaluation | NodeFlag::Evaluating));

    EXPECT_TRUE(model.isNodeEvaluated(A_id));
    EXPECT_TRUE(model.isNodeEvaluated(B_id));
    EXPECT_TRUE(model.isNodeEvaluated(E_id));

    auto A_data = model.nodeData(A_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(A_data.state, PortDataState::Valid);
    EXPECT_NE(A_data.data, nullptr);

    auto B_data = model.nodeData(B_id, PortType::Out, PortIndex(0));
    EXPECT_EQ(B_data.state, PortDataState::Valid);
    EXPECT_NE(B_data.data, nullptr);

    auto E_data = model.nodeData(E_id, PortType::In, PortIndex(0));
    EXPECT_EQ(E_data.state, PortDataState::Valid);
    EXPECT_NE(E_data.data, nullptr);

    // set C as active -> the whole graph should be evaluated
    C->setActive();

    EXPECT_TRUE(model.autoEvaluate().wait(std::chrono::seconds(1)));

    EXPECT_TRUE(model.isEvaluated());

    EXPECT_FALSE(C->nodeFlags() & (NodeFlag::RequiresEvaluation | NodeFlag::Evaluating));
    EXPECT_FALSE(D->nodeFlags() & (NodeFlag::RequiresEvaluation | NodeFlag::Evaluating));

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

    dag::debugGraph(graph.dag());

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
