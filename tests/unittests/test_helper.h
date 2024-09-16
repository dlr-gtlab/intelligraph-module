/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <gtest/gtest.h>

#include "intelli/graph.h"
#include "intelli/graphbuilder.h"
#include "intelli/data/double.h"

#include "intelli/private/utils.h"

namespace intelli
{

constexpr NodeId A_id{0};
constexpr NodeId B_id{1};
constexpr NodeId C_id{2};
constexpr NodeId D_id{3};
constexpr NodeId E_id{4};

constexpr NodeId group_id{C_id};
constexpr NodeId group_input_id{0};
constexpr NodeId group_output_id{1};
constexpr NodeId group_A_id{2};
constexpr NodeId group_B_id{3};
constexpr NodeId group_C_id{4};
constexpr NodeId group_D_id{5};

static NodeUuid A_uuid = QStringLiteral("A-UUID");
static NodeUuid B_uuid = QStringLiteral("B-UUID");
static NodeUuid C_uuid = QStringLiteral("C-UUID");
static NodeUuid D_uuid = QStringLiteral("D-UUID");
static NodeUuid E_uuid = QStringLiteral("E-UUID");

static NodeUuid group_uuid = QStringLiteral("C-UUID");
static NodeUuid group_input_uuid = QStringLiteral("C-IN-UUID");
static NodeUuid group_output_uuid = QStringLiteral("C-OUT-UUID");

static NodeUuid group_A_uuid = QStringLiteral("C-A-UUID");
static NodeUuid group_B_uuid = QStringLiteral("C-B-UUID");
static NodeUuid group_C_uuid = QStringLiteral("C-C-UUID");
static NodeUuid group_D_uuid = QStringLiteral("C-D-UUID");

namespace test
{

/** basic graph:

  .---.          .---
  | A |--26------|   |--42--.
  '---'          | C |      |
             .---|   |--O   |  .---.
             |   '---'      '--| D |
  .---.      |                 |   |--50
  | B |---8--+-----------------| + |
  '---'      |                 '---'
             |                 .---.
             '-----------------| E |
                               '---'

*/
inline bool buildBasicGraph(Graph& graph)
{
    auto modification = graph.modify();

    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), A_uuid)
                      .setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), B_uuid)
                      .setCaption(QStringLiteral("B"));
        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode"), C_uuid)
                      .setCaption(QStringLiteral("C"));
        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode"), D_uuid)
                      .setCaption(QStringLiteral("D"));
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), E_uuid)
                      .setCaption(QStringLiteral("E"));

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
        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(C.uuid(), C_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
        EXPECT_EQ(E.uuid(), E_uuid);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/** basic linear graph:

  .---.      .---.         .---.
  | A |--42--| B |      .--| C |      .---.
  '---'      |   |--42--|  |   |--84--| D |
          X--|'+'|      '--|'+'|      '---'
             '---'         '---'

 */
inline bool buildLinearGraph(Graph& graph)
{
    auto modification = graph.modify();

    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), A_uuid)
                      .setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberMathNode"), B_uuid)
                      .setCaption(QStringLiteral("B"));
        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode"), C_uuid)
                      .setCaption(QStringLiteral("C"));
        auto& D = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), D_uuid)
                      .setCaption(QStringLiteral("D"));

        builder.connect(A, PortIndex(0), B, PortIndex(0));

        builder.connect(B, PortIndex(0), C, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(1));

        builder.connect(C, PortIndex(0), D, PortIndex(0));

        setNodeProperty(A, QStringLiteral("value"), 42);

        setNodeProperty(B, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(C, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(C.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(C.uuid(), C_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/** graph with a group:

  .---.          .-------.
  | A |--26------| GROUP |--42--.
  '---'          |   C   |      |
             .---|       |--O   |  .---.
             |   '-------'      '--| D |
  .---.      |                     |   |--50
  | B |---8--+---------------------| + |
  '---'      |                     '---'
             |                     .---.
             '---------------------| E |
                                   '---'

Group C:
   .---.                                        .---.
   | A |---8----.  .---.                    .---| E |
   '---'        '--| B |                    |   '---'
                   |   |--34--.             |
  .-----.       .--| + |      |  .---.      |  .-----.
  |     |--26---'  '---'      '--| C |      |  |     |
  | IN  |                        |   |--42--+--| OUT |
  |     |---8--------------------| + |         |     |
  '-----'                        '---'         '-----'

*/
inline bool buildGraphWithGroup(Graph& graph)
{
    auto modification = graph.modify();

    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), A_uuid)
                      .setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), B_uuid)
                      .setCaption(QStringLiteral("B"));

        auto group = builder.addGraph(
            {
                typeId<DoubleData>(), // forwards to 1. port of output
                typeId<DoubleData>()  // forwards to 2. port of output
            }, {
                typeId<DoubleData>(), // connected to 1. port of D
                typeId<DoubleData>()  // not connected to any port
            },
            C_uuid,
            group_input_uuid,
            group_output_uuid
        );
        group.graph.setCaption(QStringLiteral("Group"));

        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode"), D_uuid)
                      .setCaption(QStringLiteral("D"));
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), E_uuid)
                      .setCaption(QStringLiteral("E"));

        GraphBuilder groupBuilder(group.graph);

        auto& group_A = groupBuilder.addNode(QStringLiteral("intelli::NumberSourceNode"), group_A_uuid)
                            .setCaption(QStringLiteral("Group_A"));
        auto& group_B = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode"), group_B_uuid)
                            .setCaption(QStringLiteral("Group_B"));
        auto& group_C = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode"), group_C_uuid)
                            .setCaption(QStringLiteral("Group_C"));
        auto& group_D = groupBuilder.addNode(QStringLiteral("intelli::NumberDisplayNode"), group_D_uuid)
                            .setCaption(QStringLiteral("Group_D"));

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

        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(group.graph.uuid(), C_uuid);
        EXPECT_EQ(group.graph.uuid(), group_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
        EXPECT_EQ(E.uuid(), E_uuid);

        EXPECT_EQ(group.inNode.id(), group_input_id);
        EXPECT_EQ(group.outNode.id(), group_output_id);
        EXPECT_EQ(group.inNode.uuid(), group_input_uuid);
        EXPECT_EQ(group.outNode.uuid(), group_output_uuid);

        EXPECT_EQ(group_A.uuid(), group_A_uuid);
        EXPECT_EQ(group_B.uuid(), group_B_uuid);
        EXPECT_EQ(group_C.uuid(), group_C_uuid);
        EXPECT_EQ(group_D.uuid(), group_D_uuid);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/** graph with a forwarding group:

  .---.          .-------.
  | A |--26------| GROUP |--26--.
  '---'          |   C   |      |
             .---|       |--8   |  .---.
             |   '-------'      '--| D |
  .---.      |                     |   |--34
  | B |---8--+---------------------| + |
  '---'      |                     '---'
             |                     .---.
             '---------------------| E |
                                   '---'

Group C:
  .-----.      .-----.
  |     |--26--|     |
  | IN  |      | OUT |
  |     |---8--|     |
  '-----'      '-----'

 */
inline bool buildGraphWithForwardingGroup(Graph& graph)
{
    auto modification = graph.modify();

    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), A_uuid)
                      .setCaption(QStringLiteral("A"));
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode"), B_uuid)
                      .setCaption(QStringLiteral("B"));

        auto group = builder.addGraph(
            {
                typeId<DoubleData>(), // forwards to 1. port of output
                typeId<DoubleData>()  // forwards to 2. port of output
            }, {
                typeId<DoubleData>(), // connected to 1. port of D
                typeId<DoubleData>()  // not connected to any port
            },
            C_uuid,
            group_input_uuid,
            group_output_uuid
        );
        group.graph.setCaption(QStringLiteral("C"));

        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode"), D_uuid)
                      .setCaption(QStringLiteral("D"));
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode"), E_uuid)
                      .setCaption(QStringLiteral("E"));

        {
            GraphBuilder groupBuilder(group.graph);

            // build group logic
            groupBuilder.connect(group.inNode, PortIndex{0}, group.outNode, PortIndex{0});
            groupBuilder.connect(group.inNode, PortIndex{1}, group.outNode, PortIndex{1});
        }

        builder.connect(A, PortIndex{0}, group.graph, PortIndex{0});
        builder.connect(B, PortIndex{0}, group.graph, PortIndex{1});

        builder.connect(group.graph, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);
        setNodeProperty(D, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(group.graph.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);

        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(group.graph.uuid(), C_uuid);
        EXPECT_EQ(group.graph.uuid(), group_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
        EXPECT_EQ(E.uuid(), E_uuid);

        EXPECT_EQ(group.inNode.id(), group_input_id);
        EXPECT_EQ(group.outNode.id(), group_output_id);
        EXPECT_EQ(group.inNode.uuid(), group_input_uuid);
        EXPECT_EQ(group.outNode.uuid(), group_output_uuid);
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
