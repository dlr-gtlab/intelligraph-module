/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <gtest/gtest.h>

#include <intelli/globals.h>


TEST(Globals, quantize)
{
    using namespace intelli;

    EXPECT_EQ(quantize(QPointF(42.4,  9.75), 5), QPoint(40,  10));
    EXPECT_EQ(quantize(QPointF(42.7, -9.75), 5), QPoint(40, -10));
    EXPECT_EQ(quantize(QPointF(43.0, -9.75), 5), QPoint(45, -10));
}

TEST(Globals, connection_is_valid)
{
    using namespace intelli;

    ConnectionId id{NodeId(0), PortId(0), NodeId(1), PortId(0)};
    EXPECT_TRUE(id.isValid());

    id.outNodeId = NodeId{};
    EXPECT_FALSE(id.isValid());
}

TEST(Globals, connection_reversed)
{
    using namespace intelli;

    ConnectionId ida{NodeId(0), PortId(1), NodeId(1), PortId(0)};
    ConnectionId idb{NodeId(1), PortId(0), NodeId(0), PortId(1)};
    EXPECT_NE(ida, idb);
    EXPECT_EQ(ida.reversed(), idb);
    ida.reverse();
    EXPECT_EQ(ida, idb);
}

TEST(Globals, connection_node)
{
    using namespace intelli;

    ConnectionId id{NodeId(0), PortId(1), NodeId(1), PortId(0)};

    EXPECT_EQ(id.node(PortType::In),  id.inNodeId);
    EXPECT_EQ(id.node(PortType::Out), id.outNodeId);
    EXPECT_EQ(id.port(PortType::In),  id.inPort);
    EXPECT_EQ(id.port(PortType::Out), id.outPort);
}

TEST(Globals, connection_draft)
{
    using namespace intelli;

    ConnectionId id{NodeId(0), PortId(1), NodeId(1), PortId(0)};

    EXPECT_FALSE(id.isDraft());
    EXPECT_EQ(id.draftType(), PortType::NoType);
    id.inNodeId = NodeId{};
    EXPECT_FALSE(id.isDraft());
    EXPECT_EQ(id.draftType(), PortType::NoType);

    id.inPort   = PortId{};
    EXPECT_TRUE(id.isDraft());

    EXPECT_EQ(id.draftType(), PortType::Out);
    EXPECT_EQ(id.reversed().draftType(), PortType::In);
}
