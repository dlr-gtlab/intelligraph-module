/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <gtest/gtest.h>

#include "intelli/graph.h"
#include "intelli/graphbuilder.h"
#include "intelli/data/double.h"

namespace intelli
{

constexpr NodeId A_id{0};
constexpr NodeId B_id{1};
constexpr NodeId C_id{2};
constexpr NodeId D_id{3};
constexpr NodeId E_id{4};

constexpr NodeId group_input_id{0};
constexpr NodeId group_output_id{1};
constexpr NodeId group_A_id{2};
constexpr NodeId group_B_id{3};
constexpr NodeId group_C_id{4};
constexpr NodeId group_D_id{5};

namespace test
{

inline bool buildBasicGraph(Graph& graph)
{
    GraphBuilder builder(graph);

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));

        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));
        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));

        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));

        // square value 1
        builder.connect(A, PortIndex{0}, C, PortIndex{0});
        builder.connect(B, PortIndex{0}, C, PortIndex{1});

        // multiply value 2 by result of square
        builder.connect(C, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);

        setNodeProperty(C, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(D, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(C.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

inline bool buildLinearGraph(Graph& graph)
{
    GraphBuilder builder(graph);

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("B"));
        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));

        builder.connect(A, PortIndex(0), B, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(1));

        setNodeProperty(A, QStringLiteral("value"), 42);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

inline bool buildGraphWithGroup(Graph& graph)
{
    GraphBuilder builder(graph);

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));

        auto group = builder.addGraph({
                typeId<DoubleData>(),
                typeId<DoubleData>()
            }, {
                typeId<DoubleData>()
            }
        );
        group.graph.setCaption(QStringLiteral("Group"));

        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));

        GraphBuilder groupBuilder(group.graph);

        auto& group_A = groupBuilder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("Group_A"));
        auto& group_B = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("Group_B"));
        auto& group_C = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("Group_C"));
        auto& group_D = groupBuilder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("Group_D"));

        // square value 1
        builder.connect(A, PortIndex{0}, group.graph, PortIndex{0});
        builder.connect(B, PortIndex{0}, group.graph, PortIndex{1});

        // build group logic
        groupBuilder.connect(group_A, PortIndex{0}, group_B, PortIndex{0});
        groupBuilder.connect(group.inNode, PortIndex{0}, group_B, PortIndex{1});

        groupBuilder.connect(group_B, PortIndex{0}, group_C, PortIndex{0});
        groupBuilder.connect(group.inNode, PortIndex{1}, group_C, PortIndex{1});

        groupBuilder.connect(group_C, PortIndex{0}, group.outNode, PortIndex{0});

        groupBuilder.connect(group_C, PortIndex{0}, group_D, PortIndex{0});

        builder.connect(group.graph, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);
        setNodeProperty(group_A, QStringLiteral("value"), 8);

        setNodeProperty(group_B, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(D,       QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(group.graph.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);
        EXPECT_EQ(group_A.id(), group_A_id);
        EXPECT_EQ(group_B.id(), group_B_id);
        EXPECT_EQ(group_C.id(), group_C_id);
        EXPECT_EQ(group_D.id(), group_D_id);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

} // namespace test

} // namespace intelli

#endif // TEST_HELPER_H
