/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 12.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
