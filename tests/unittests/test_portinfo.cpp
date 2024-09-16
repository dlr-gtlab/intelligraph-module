/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */


#include <gtest/gtest.h>

#include "intelli/data/double.h"

#include "node/test_node.h"

TEST(PortInfo, default_id)
{
    using namespace intelli;

    auto port = Node::PortInfo(typeId<DoubleData>());
    TestNode node;
    auto id = node.addInPort(port);
    EXPECT_EQ(id, 0);
}

TEST(PortInfo, custom_id)
{
    using namespace intelli;

    auto port = Node::PortInfo::customId(PortId(42), typeId<DoubleData>());
    TestNode node;
    auto id = node.addInPort(port);
    EXPECT_EQ(id, 42);
}

TEST(PortInfo, copy)
{
    using namespace intelli;

    auto port = Node::PortInfo::customId(PortId(42), typeId<DoubleData>());

    ASSERT_EQ(port.id(), 42);

    auto portDefaultCopy = port;
    auto portCustomCopy = port.copy();

    EXPECT_EQ(portDefaultCopy.id(), port.id());
    EXPECT_EQ(portCustomCopy.id(), invalid<PortId>());
}
