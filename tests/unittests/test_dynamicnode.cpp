/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 9.2.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include <intelli/data/bool.h>
#include <intelli/data/double.h>

#include "test_helper.h"

#include "node/test_dynamic.h"


TEST(DynamicNode, no_white_list)
{
    TestDynamicWhiteListNode n;

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());

    ASSERT_EQ(n.ports(intelli::PortType::In).size(),  2);
    EXPECT_TRUE(a != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(b != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::BoolData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(), 2);
    EXPECT_TRUE(c != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(d != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());
}

TEST(DynamicNode, white_list_input_only)
{
    QStringList inputWhiteList = { intelli::typeId<intelli::DoubleData>() };
    TestDynamicWhiteListNode n{inputWhiteList};

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());

    // adding is allowed, but invalid typeId is converted into valid typeId
    ASSERT_EQ(n.ports(intelli::PortType::In).size(),  2);
    EXPECT_TRUE(a != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(b != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::DoubleData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(),  2);
    EXPECT_TRUE(c != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(d != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());
}

TEST(DynamicNode, white_list_input_and_output)
{
    QStringList inputWhiteList = { intelli::typeId<intelli::DoubleData>() };
    QStringList outputWhiteList = { intelli::typeId<intelli::BoolData>() };
    TestDynamicWhiteListNode n{inputWhiteList, outputWhiteList};

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());

    // adding is allowed, but invalid typeId is converted into valid typeId
    ASSERT_EQ(n.ports(intelli::PortType::In).size(),  2);
    EXPECT_TRUE(a != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(b != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::DoubleData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(),  2);
    EXPECT_TRUE(c != intelli::invalid<intelli::PortId>());
    EXPECT_TRUE(d != intelli::invalid<intelli::PortId>());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::BoolData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());
}
