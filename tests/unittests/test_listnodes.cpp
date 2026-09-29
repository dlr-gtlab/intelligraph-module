/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <gtest/gtest.h>

#include <intelli/data/int.h>
#include <intelli/data/stringlist.h>
#include <intelli/data/list.h>

#include <intelli/graph.h>
#include <intelli/graphbuilder.h>
#include <intelli/graphexecmodel.h>


class IntListNodes : public ::testing::Test
{
public:

    void SetUp() override
    {
        intelli::GraphBuilder builder{graph};
        intelli::Node& inputA = builder.addNode("intelli::IntInputNode");
        intelli::Node& inputB = builder.addNode("intelli::IntInputNode");
        intelli::Node& inputC = builder.addNode("intelli::IntInputNode");
        intelli::Node& index = builder.addNode("intelli::IntInputNode");
        intelli::setNodeProperty(inputA, "value", 5);
        intelli::setNodeProperty(inputB, "value", 30);
        intelli::setNodeProperty(inputC, "value", 7);
        intelli::setNodeProperty(index, "value", 2);

        intelli::Node& toListNode = builder.addNode("intelli::ToListNode");
        intelli::DynamicNode* toListPtr = qobject_cast<intelli::DynamicNode*>(&toListNode);
        assert(toListPtr);
        intelli::DynamicNode& toList = *toListPtr;

        intelli::setNodeProperty(toList, "typeId", intelli::typeId<intelli::IntData>());
        assert(toList.ports(intelli::PortType::In).size() == 1);
        toList.addInPort(intelli::makePort(intelli::listTypeId<intelli::IntData>()));
        toList.addInPort(intelli::makePort(intelli::listTypeId<intelli::IntData>()));

        builder.connect(inputA, intelli::PortIndex{0}, toList, intelli::PortIndex{0});
        builder.connect(inputB, intelli::PortIndex{0}, toList, intelli::PortIndex{1});
        builder.connect(inputC, intelli::PortIndex{0}, toList, intelli::PortIndex{2});

        intelli::Node& getSize = builder.addNode("intelli::ListGetSizeNode");
        intelli::Node& getEntry = builder.addNode("intelli::ListGetIndexNode");

        intelli::setNodeProperty(getSize, "typeId", intelli::typeId<intelli::IntData>());
        intelli::setNodeProperty(getEntry, "typeId", intelli::typeId<intelli::IntData>());

        builder.connect(toList, intelli::PortIndex{0}, getSize, intelli::PortIndex{0});
        builder.connect(toList, intelli::PortIndex{0}, getEntry, intelli::PortIndex{0});
        builder.connect(index, intelli::PortIndex{0}, getEntry, intelli::PortIndex{1});

        toListUuid = toList.uuid();
        getSizeUuid = getSize.uuid();
        getEntryUuid = getEntry.uuid();
    }

    void TearDown() override
    {
        graph.clearGraph();
    }

    intelli::Graph graph;
    intelli::NodeUuid toListUuid, getSizeUuid, getEntryUuid;
};

TEST_F(IntListNodes, verify)
{
    constexpr int expectedSize = 3;
    constexpr int expectedOutput = 7;

    intelli::GraphExecutionModel exec{graph};
    auto toListEvaluated = exec.evaluateGraph();
    ASSERT_TRUE(toListEvaluated.startedSuccessfully());
    ASSERT_TRUE(toListEvaluated.wait(std::chrono::seconds{1}));

    ASSERT_TRUE(exec.isNodeEvaluated(toListUuid));
    auto listData = exec.nodeData(toListUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::BaseListData>();
    ASSERT_TRUE(listData);
    ASSERT_EQ(listData->iterate().size(), expectedSize);

    intelli::Ptr<intelli::IntData> entry0 = qobject_pointer_cast<intelli::IntData const>(*std::next(listData->iterate().begin(), 0));
    intelli::Ptr<intelli::IntData> entry1 = qobject_pointer_cast<intelli::IntData const>(*std::next(listData->iterate().begin(), 1));
    intelli::Ptr<intelli::IntData> entry2 = qobject_pointer_cast<intelli::IntData const>(*std::next(listData->iterate().begin(), 2));

    ASSERT_EQ(entry0->value(), 5);
    ASSERT_EQ(entry1->value(), 30);
    ASSERT_EQ(entry2->value(), 7);

    ASSERT_TRUE(exec.isNodeEvaluated(getSizeUuid));
    auto sizeData = exec.nodeData(getSizeUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::IntData>();
    ASSERT_TRUE(sizeData);
    ASSERT_EQ(sizeData->value(), expectedSize);

    ASSERT_TRUE(exec.isNodeEvaluated(getEntryUuid));
    auto entryData = exec.nodeData(getEntryUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::IntData>();
    ASSERT_TRUE(entryData);
    ASSERT_EQ(entryData->value(), expectedOutput);
    ASSERT_EQ(entryData->value(), entry2->value());
}

class StringListNodes : public ::testing::Test
{
public:

    void SetUp() override
    {
        intelli::GraphBuilder builder{graph};
        intelli::Node& inputA = builder.addNode("intelli::StringInputNode");
        intelli::Node& inputB = builder.addNode("intelli::StringInputNode");
        intelli::Node& inputC = builder.addNode("intelli::StringInputNode");
        intelli::Node& index = builder.addNode("intelli::IntInputNode");
        intelli::setNodeProperty(inputA, "value", "5");
        intelli::setNodeProperty(inputB, "value", "30");
        intelli::setNodeProperty(inputC, "value", "7");
        intelli::setNodeProperty(index, "value", 2);

        intelli::Node& toListNode = builder.addNode("intelli::ToListNode");
        intelli::DynamicNode* toListPtr = qobject_cast<intelli::DynamicNode*>(&toListNode);
        assert(toListPtr);
        intelli::DynamicNode& toList = *toListPtr;

        intelli::setNodeProperty(toList, "typeId", intelli::typeId<intelli::StringData>());
        assert(toList.ports(intelli::PortType::In).size() == 1);
        toList.addInPort(intelli::makePort(intelli::listTypeId<intelli::StringData>()));
        toList.addInPort(intelli::makePort(intelli::listTypeId<intelli::StringData>()));

        builder.connect(inputA, intelli::PortIndex{0}, toList, intelli::PortIndex{0});
        builder.connect(inputB, intelli::PortIndex{0}, toList, intelli::PortIndex{1});
        builder.connect(inputC, intelli::PortIndex{0}, toList, intelli::PortIndex{2});

        intelli::Node& getSize = builder.addNode("intelli::ListGetSizeNode");
        intelli::Node& getEntry = builder.addNode("intelli::ListGetIndexNode");

        intelli::setNodeProperty(getSize, "typeId", intelli::typeId<intelli::StringData>());
        intelli::setNodeProperty(getEntry, "typeId", intelli::typeId<intelli::StringData>());

        builder.connect(toList, intelli::PortIndex{0}, getSize, intelli::PortIndex{0});
        builder.connect(toList, intelli::PortIndex{0}, getEntry, intelli::PortIndex{0});
        builder.connect(index, intelli::PortIndex{0}, getEntry, intelli::PortIndex{1});

        toListUuid = toList.uuid();
        getSizeUuid = getSize.uuid();
        getEntryUuid = getEntry.uuid();
    }

    void TearDown() override
    {
        graph.clearGraph();
    }

    intelli::Graph graph;
    intelli::NodeUuid toListUuid, getSizeUuid, getEntryUuid;
};

TEST_F(StringListNodes, verify)
{
    constexpr int expectedSize = 3;
    QString expectedOutput = "7";

    intelli::GraphExecutionModel exec{graph};
    auto toListEvaluated = exec.evaluateGraph();
    ASSERT_TRUE(toListEvaluated.startedSuccessfully());
    ASSERT_TRUE(toListEvaluated.wait(std::chrono::seconds{1}));

    ASSERT_TRUE(exec.isNodeEvaluated(toListUuid));
    auto listData = exec.nodeData(toListUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::BaseListData>();
    ASSERT_TRUE(listData);
    ASSERT_EQ(listData->iterate().size(), expectedSize);

    auto entry0 = qobject_pointer_cast<intelli::StringData const>(*std::next(listData->iterate().begin(), 0));
    auto entry1 = qobject_pointer_cast<intelli::StringData const>(*std::next(listData->iterate().begin(), 1));
    auto entry2 = qobject_pointer_cast<intelli::StringData const>(*std::next(listData->iterate().begin(), 2));

    ASSERT_EQ(entry0->value(), "5");
    ASSERT_EQ(entry1->value(), "30");
    ASSERT_EQ(entry2->value(), "7");

    ASSERT_TRUE(exec.isNodeEvaluated(getSizeUuid));
    auto sizeData = exec.nodeData(getSizeUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::IntData>();
    ASSERT_TRUE(sizeData);
    ASSERT_EQ(sizeData->value(), expectedSize);

    ASSERT_TRUE(exec.isNodeEvaluated(getEntryUuid));
    auto entryData = exec.nodeData(getEntryUuid, intelli::PortType::Out, intelli::PortIndex{0}).as<intelli::StringData>();
    ASSERT_TRUE(entryData);
    ASSERT_EQ(entryData->value(), expectedOutput);
    ASSERT_EQ(entryData->value(), entry2->value());
}
