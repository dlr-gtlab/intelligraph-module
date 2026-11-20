/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/data/bool.h>
#include <intelli/data/double.h>
#include <intelli/data/int.h>
#include <intelli/data/string.h>
#include <intelli/nodedatafactory.h>

#include "test_helper.h"

#include "node/test_dynamic.h"

// TODO: test memento restore with Input/Output only

namespace
{

QStringList getAllTypeIds()
{
    auto typeIds = intelli::NodeDataFactory::instance().registeredTypeIds();
    std::sort(typeIds.begin(), typeIds.end());
    return typeIds;
}

} // namespace

TEST(DynamicNode, static_ports)
{
    TestDynamicWhiteListNode n{intelli::DynamicNode::DynamicInputAndOutput};

    intelli::PortId a = n.addStaticInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addStaticInPort(intelli::typeId<intelli::IntData>());
    intelli::PortId d = n.addInPort(intelli::typeId<intelli::StringData>());
    intelli::PortId e = n.addStaticInPort(intelli::typeId<intelli::IntData>());

    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 5);
    EXPECT_TRUE(a.isValid());
    EXPECT_TRUE(b.isValid());
    EXPECT_TRUE(c.isValid());
    EXPECT_TRUE(d.isValid());
    EXPECT_TRUE(e.isValid());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::BoolData>());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::IntData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::StringData>());
    EXPECT_EQ(n.port(e)->typeId, intelli::typeId<intelli::IntData>());
    EXPECT_EQ(n.portIndex(a).idx, 0);
    EXPECT_EQ(n.portIndex(c).idx, 1);
    EXPECT_EQ(n.portIndex(e).idx, 2);
    EXPECT_EQ(n.portIndex(b).idx, 3); // static ports are grouped together
    EXPECT_EQ(n.portIndex(d).idx, 4);

    EXPECT_FALSE(n.isDynamicPort(a));
    EXPECT_FALSE(n.isDynamicPort(c));
    EXPECT_FALSE(n.isDynamicPort(e));
    EXPECT_TRUE(n.isDynamicPort(b));
    EXPECT_TRUE(n.isDynamicPort(d));

    n.removePort(c);
    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 4);
    EXPECT_EQ(n.portIndex(a).idx, 0);
    EXPECT_FALSE(n.portIndex(c));
    EXPECT_EQ(n.portIndex(e).idx, 1);
    EXPECT_EQ(n.portIndex(b).idx, 2);
    EXPECT_EQ(n.portIndex(d).idx, 3);

    n.removePort(b);
    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 3);
    EXPECT_EQ(n.portIndex(a).idx, 0);
    EXPECT_FALSE(n.portIndex(c));
    EXPECT_EQ(n.portIndex(e).idx, 1);
    EXPECT_FALSE(n.portIndex(b));
    EXPECT_EQ(n.portIndex(d).idx, 2);
}

TEST(DynamicNode, no_white_list)
{
    TestDynamicWhiteListNode n{intelli::DynamicNode::DynamicInputAndOutput};

    EXPECT_EQ(n.inputWhitelist(), ::getAllTypeIds());
    EXPECT_EQ(n.outputWhitelist(), ::getAllTypeIds());

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId s1 = n.addStaticInPort(intelli::typeId<intelli::StringData>());
    intelli::PortId s2 = n.addStaticOutPort(intelli::typeId<intelli::StringData>());

    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 3);
    EXPECT_TRUE(a.isValid());
    EXPECT_TRUE(b.isValid());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::BoolData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(), 3);
    EXPECT_TRUE(c.isValid());
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());

    // static ports bypass whitelists
    EXPECT_TRUE(s1.isValid());
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(n.port(s1)->typeId, intelli::typeId<intelli::StringData>());
    EXPECT_EQ(n.port(s2)->typeId, intelli::typeId<intelli::StringData>());
}

TEST(DynamicNode, white_list_input_only)
{
    QStringList inputWhiteList = { intelli::typeId<intelli::DoubleData>() };
    TestDynamicWhiteListNode n{
        intelli::DynamicNode::DynamicInputAndOutput, inputWhiteList
    };

    EXPECT_EQ(n.inputWhitelist(), inputWhiteList);
    EXPECT_EQ(n.outputWhitelist(), ::getAllTypeIds());

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId s1 = n.addStaticInPort(intelli::typeId<intelli::StringData>());
    intelli::PortId s2 = n.addStaticOutPort(intelli::typeId<intelli::StringData>());

    // adding is allowed, but invalid typeId is converted into valid typeId
    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 3);
    EXPECT_TRUE(a.isValid());
    EXPECT_TRUE(b.isValid());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::DoubleData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(), 3);
    EXPECT_TRUE(c.isValid());
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());

    // static ports bypass whitelists
    EXPECT_TRUE(s1.isValid());
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(n.port(s1)->typeId, intelli::typeId<intelli::StringData>());
    EXPECT_EQ(n.port(s2)->typeId, intelli::typeId<intelli::StringData>());
}

TEST(DynamicNode, white_list_output_only)
{
    QStringList outputWhiteList = { intelli::typeId<intelli::IntData>() };
    TestDynamicWhiteListNode n{
        intelli::DynamicNode::DynamicInputAndOutput, {}, outputWhiteList
    };

    EXPECT_EQ(n.inputWhitelist(), ::getAllTypeIds());
    EXPECT_EQ(n.outputWhitelist(), outputWhiteList);

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId s1 = n.addStaticInPort(intelli::typeId<intelli::StringData>());
    intelli::PortId s2 = n.addStaticOutPort(intelli::typeId<intelli::StringData>());

    // adding is allowed, but invalid typeId is converted into valid typeId
    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 3);
    EXPECT_TRUE(a.isValid());
    EXPECT_TRUE(b.isValid());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::BoolData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(), 3);
    EXPECT_TRUE(c.isValid());
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::IntData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::IntData>());

    // static ports bypass whitelists
    EXPECT_TRUE(s1.isValid());
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(n.port(s1)->typeId, intelli::typeId<intelli::StringData>());
    EXPECT_EQ(n.port(s2)->typeId, intelli::typeId<intelli::StringData>());
}

TEST(DynamicNode, white_list_input_and_output)
{
    QStringList inputWhiteList = { intelli::typeId<intelli::DoubleData>() };
    QStringList outputWhiteList = { intelli::typeId<intelli::BoolData>() };
    TestDynamicWhiteListNode n{
        intelli::DynamicNode::DynamicInputAndOutput, inputWhiteList, outputWhiteList
    };

    EXPECT_EQ(n.inputWhitelist(), inputWhiteList);
    EXPECT_EQ(n.outputWhitelist(), outputWhiteList);

    intelli::PortId a = n.addInPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId b = n.addInPort(intelli::typeId<intelli::BoolData>());
    intelli::PortId c = n.addOutPort(intelli::typeId<intelli::DoubleData>());
    intelli::PortId d = n.addOutPort(intelli::typeId<intelli::IntData>());
    intelli::PortId s1 = n.addStaticInPort(intelli::typeId<intelli::StringData>());
    intelli::PortId s2 = n.addStaticOutPort(intelli::typeId<intelli::StringData>());

    // adding is allowed, but invalid typeId is converted into valid typeId
    ASSERT_EQ(n.ports(intelli::PortType::In).size(), 3);
    EXPECT_TRUE(a.isValid());
    EXPECT_TRUE(b.isValid());
    EXPECT_EQ(n.port(a)->typeId, intelli::typeId<intelli::DoubleData>());
    EXPECT_EQ(n.port(b)->typeId, intelli::typeId<intelli::DoubleData>());

    ASSERT_EQ(n.ports(intelli::PortType::Out).size(), 3);
    EXPECT_TRUE(c.isValid());
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(n.port(c)->typeId, intelli::typeId<intelli::BoolData>());
    EXPECT_EQ(n.port(d)->typeId, intelli::typeId<intelli::BoolData>());

    // static ports bypass whitelists
    EXPECT_TRUE(s1.isValid());
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(n.port(s1)->typeId, intelli::typeId<intelli::StringData>());
    EXPECT_EQ(n.port(s2)->typeId, intelli::typeId<intelli::StringData>());
}
