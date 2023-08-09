/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 9.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */
#include "intelli/data/double.h"
#include <gtest/gtest.h>

#include <intelli/graph.h>
#include <intelli/graphbuilder.h>

TEST(GraphBuilder, numbers)
{
    intelli::Graph graph;

    intelli::GraphBuilder builder(graph);

    intelli::Node* resultNode = nullptr;

    try
    {

        auto& value1   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"));
        auto& value2   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"));

        auto& square   = builder.addNode(QStringLiteral("intelli::test::NumberMathNode"));
        auto& multiply = builder.addNode(QStringLiteral("intelli::test::NumberMathNode"));
        auto& add      = builder.addNode(QStringLiteral("intelli::test::NumberMathNode"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"));

        // square value 1
        builder.connect(value1, intelli::PortIndex{0}, square, intelli::PortIndex{0});
        builder.connect(value1, intelli::PortIndex{0}, square, intelli::PortIndex{1});

        // multiply value 2 by result of square
        builder.connect(value2, intelli::PortIndex{0}, multiply, intelli::PortIndex{0});
        builder.connect(square, intelli::PortIndex{0}, multiply, intelli::PortIndex{1});

        // add result of multiply and value 1 together
        builder.connect(multiply, intelli::PortIndex{0}, add, intelli::PortIndex{0});
        builder.connect(value1,   intelli::PortIndex{0}, add, intelli::PortIndex{1});

        // forward result of add to display
        builder.connect(add, intelli::PortIndex{0}, result, intelli::PortIndex{0});

        // set values
        intelli::setNodeProperty(value1, QStringLiteral("value"), 2);
        intelli::setNodeProperty(value2, QStringLiteral("value"), 10);

        intelli::setNodeProperty(square,   QStringLiteral("operation"), QStringLiteral("Multiply"));
        intelli::setNodeProperty(multiply, QStringLiteral("operation"), QStringLiteral("Multiply"));
        intelli::setNodeProperty(add,      QStringLiteral("operation"), QStringLiteral("Plus"));

        resultNode = &result;
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        ASSERT_NO_THROW(throw);
        return;
    }

    auto success = intelli::evaluate(graph);
    EXPECT_TRUE(success);

    // access result
    ASSERT_NE(resultNode, nullptr);

    auto resultData = resultNode->inData(intelli::PortIndex{0});
    ASSERT_NE(resultData, nullptr);

    auto resultValue = qobject_cast<intelli::DoubleData const*>(resultData.get());
    ASSERT_NE(resultValue, nullptr);
    EXPECT_EQ(resultValue->value(), 42);
}

TEST(GraphBuilder, group)
{
    intelli::Graph graph;

    intelli::GraphBuilder builder(graph);

    intelli::Node* resultNode = nullptr;

    try
    {

        auto& value1   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"));
        auto& value2   = builder.addNode(QStringLiteral("intelli::NumberSourceNode"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"));

        auto sub = builder.addSubGraph({intelli::typeId<intelli::DoubleData>(),
                                        intelli::typeId<intelli::DoubleData>()},
                                       {intelli::typeId<intelli::DoubleData>()});

        intelli::GraphBuilder subBuilder(sub.graph);

        auto& add = subBuilder.addNode(QStringLiteral("intelli::test::NumberMathNode"));

        // connect values to sub graph
        builder.connect(value1, intelli::PortIndex{0}, sub.graph, intelli::PortIndex{0});
        builder.connect(value2, intelli::PortIndex{0}, sub.graph, intelli::PortIndex{1});

        // connect inputs to add node
        subBuilder.connect(sub.input, intelli::PortIndex{0}, add, intelli::PortIndex{0});
        subBuilder.connect(sub.input, intelli::PortIndex{1}, add, intelli::PortIndex{1});

        // connect results from add node to output
        subBuilder.connect(add, intelli::PortIndex{0}, sub.output, intelli::PortIndex{0});

        // forward result of sub graph to display
        builder.connect(sub.graph, intelli::PortIndex{0}, result, intelli::PortIndex{0});

        // set values
        intelli::setNodeProperty(value1, QStringLiteral("value"), 16);
        intelli::setNodeProperty(value2, QStringLiteral("value"), 26);

        intelli::setNodeProperty(add, QStringLiteral("operation"), QStringLiteral("Plus"));

        resultNode = &result;
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        ASSERT_NO_THROW(throw);
        return;
    }

    auto success = intelli::evaluate(graph);
    EXPECT_TRUE(success);

    // access result
    ASSERT_NE(resultNode, nullptr);

    auto resultData = resultNode->inData(intelli::PortIndex{0});
    ASSERT_NE(resultData, nullptr);

    auto resultValue = qobject_cast<intelli::DoubleData const*>(resultData.get());
    ASSERT_NE(resultValue, nullptr);
    EXPECT_EQ(resultValue->value(), 42);
}
