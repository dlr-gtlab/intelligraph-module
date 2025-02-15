/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <gtest/gtest.h>

#include <intelli/data/double.h>
#include <intelli/graph.h>
#include <intelli/graphbuilder.h>
#include <intelli/graphexecmodel.h>

using namespace intelli;

TEST(GraphBuilder, basic_graph)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {

        auto& value1   = builder.addNode(QStringLiteral("TestNumberInputNode"));
        auto& value2   = builder.addNode(QStringLiteral("TestNumberInputNode"));

        auto& square   = builder.addNode(QStringLiteral("intelli::NumberMathNode"));
        auto& multiply = builder.addNode(QStringLiteral("intelli::NumberMathNode"));
        auto& add      = builder.addNode(QStringLiteral("intelli::NumberMathNode"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"));

        // square value 1
        builder.connect(value1, PortIndex{0}, square, PortIndex{0});
        builder.connect(value1, PortIndex{0}, square, PortIndex{1});

        // multiply value 2 by result of square
        builder.connect(value2, PortIndex{0}, multiply, PortIndex{0});
        builder.connect(square, PortIndex{0}, multiply, PortIndex{1});

        // add result of multiply and value 1 together
        builder.connect(multiply, PortIndex{0}, add, PortIndex{0});
        builder.connect(value1,   PortIndex{0}, add, PortIndex{1});

        // forward result of add to display
        builder.connect(add, PortIndex{0}, result, PortIndex{0});

        // set values
        setNodeProperty(value1, QStringLiteral("value"), 2);
        setNodeProperty(value2, QStringLiteral("value"), 10);

        setNodeProperty(square,   QStringLiteral("operation"), QStringLiteral("Multiply"));
        setNodeProperty(multiply, QStringLiteral("operation"), QStringLiteral("Multiply"));
        setNodeProperty(add,      QStringLiteral("operation"), QStringLiteral("Plus"));

    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        ASSERT_NO_THROW(throw);
        return;
    }

    debug(graph);
}

TEST(GraphBuilder, graph_with_groups)
{
    Graph graph;

    GraphBuilder builder(graph);

    try
    {

        auto& value1   = builder.addNode(QStringLiteral("TestNumberInputNode"));
        auto& value2   = builder.addNode(QStringLiteral("TestNumberInputNode"));

        auto& result = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"));

        auto group = builder.addGraph({
            typeId<DoubleData>(),
            typeId<DoubleData>()
        }, {
            typeId<DoubleData>()
        });

        GraphBuilder groupBuilder(group.graph);

        auto& add = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode"));

        // connect values to sub graph
        builder.connect(value1, PortIndex{0}, group.graph, PortIndex{0});
        builder.connect(value2, PortIndex{0}, group.graph, PortIndex{1});

        // connect inputs to add node
        groupBuilder.connect(group.inNode, PortIndex{0}, add, PortIndex{0});
        groupBuilder.connect(group.inNode, PortIndex{1}, add, PortIndex{1});

        // connect results from add node to output
        groupBuilder.connect(add, PortIndex{0}, group.outNode, PortIndex{0});

        // forward result of sub graph to display
        builder.connect(group.graph, PortIndex{0}, result, PortIndex{0});

        // set values
        setNodeProperty(value1, QStringLiteral("value"), 16);
        setNodeProperty(value2, QStringLiteral("value"), 26);

        setNodeProperty(add, QStringLiteral("operation"), QStringLiteral("Plus"));

    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        ASSERT_NO_THROW(throw);
        return;
    }

    debug(graph);
}
