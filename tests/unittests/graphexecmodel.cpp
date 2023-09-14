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

    GraphBuilder builder(graph);

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("B"));
        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));

        builder.connect(A, PortIndex(0), B, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(1));

        // set in port 2 of node B to required thus graph cannot be evaluated
//        B.port(B.portId(PortType::In, PortIndex(1)))->optional = false;

        setNodeProperty(A, QStringLiteral("value"), 42);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        ASSERT_NO_THROW(throw e);
    }

    dag::debugGraph(graph.dag());

    GraphExecutionModel model(graph);

    EXPECT_TRUE(model.evaluateNode(C_id));
    EXPECT_TRUE(model.waitForNode(std::chrono::seconds{1}));

    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 84);

    EXPECT_TRUE(model.evaluateNode(C_id));
}

TEST(GraphExecutionModel, dependencie)
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

    EXPECT_TRUE(model.autoEvaluate());
    EXPECT_TRUE(model.wait(std::chrono::seconds{1}));

    auto B_data = model.nodeData(B_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(B_data);
    EXPECT_EQ(B_data->value(), 42);
}

TEST(GraphExecutionModel, auto_evaluate_basic_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_TRUE(isAcyclic(graph));

    dag::debugGraph(graph.dag());

    // auto evaluate

    GraphExecutionModel model(graph);

    EXPECT_FALSE(model.evaluated());

    EXPECT_TRUE(model.autoEvaluate());
    EXPECT_TRUE(model.wait(std::chrono::seconds(1)));

    auto D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 42);

    auto E_data = model.nodeData(E_id, PortType::In, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(E_data);
    EXPECT_EQ(E_data->value(), 8);

    EXPECT_TRUE(model.evaluated());

    // disable auto evaluation

    gtDebug() << "";

    EXPECT_TRUE(model.autoEvaluate(false));

    model.setNodeData(A_id, PortType::Out, PortIndex(0), std::make_shared<DoubleData>(12));

    EXPECT_FALSE(model.evaluated());

    // old values are still set
    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 42);

    // not auto evaluating
    EXPECT_FALSE(model.wait(std::chrono::seconds(1)));

    // reenable auto evaluation

    gtDebug() << "";

    EXPECT_TRUE(model.autoEvaluate());
    EXPECT_TRUE(model.wait(std::chrono::seconds(1)));

    // new values is set
    D_data = model.nodeData(D_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 28);

    EXPECT_TRUE(model.evaluated());
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
    GraphExecutionModel* submodel = graph.mainExecutionModel();
    ASSERT_EQ(submodel, &model);

    EXPECT_TRUE(model.autoEvaluate());
    EXPECT_TRUE(model.wait(std::chrono::seconds(1)));

    auto C_data = model.nodeData(C_id, PortType::Out, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(C_data);
    EXPECT_EQ(C_data->value(), 34);

    auto D_data = model.nodeData(E_id, PortType::In, PortIndex(0)).cast<DoubleData>();
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 8);
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

    EXPECT_FALSE(model.autoEvaluate());
    EXPECT_FALSE(model.wait(std::chrono::seconds(1)));
}
