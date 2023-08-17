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


TEST(GraphExecutionModel, basic_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildTestGraph(graph));

    EXPECT_TRUE(isAcyclic(graph));

    dag::debugGraph(graph.dag());

    GraphExecutionModel model(graph);

    auto success = model.autoEvaluate();
    EXPECT_TRUE(success);

    auto D_data = qobject_pointer_cast<DoubleData const>(model.nodeData(D_id, PortType::Out, PortIndex(0)));
    ASSERT_TRUE(D_data);
    EXPECT_EQ(D_data->value(), 42);

    auto E_data = qobject_pointer_cast<DoubleData const>(model.nodeData(E_id, PortType::In, PortIndex(0)));
    ASSERT_TRUE(E_data);
    EXPECT_EQ(E_data->value(), 8);
}


TEST(GraphExecutionModel, basic_graph_with_cycle)
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

    auto success = evaluate(graph);
    EXPECT_FALSE(success);
}
