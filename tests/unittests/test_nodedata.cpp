/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"

#include "data/test_nodedata.h"

#include <intelli/nodedatafactory.h>
#include <intelli/data/file.h>

#include <QFileInfo>

using namespace intelli;

TEST(NodeData, sanity_check)
{
    TestNodeData data{42};

    ASSERT_DOUBLE_EQ(data.myDouble(), 42);
    ASSERT_DOUBLE_EQ(data.myDoubleModified(2, QStringLiteral("test")), 42 * 2 * 4);
}

TEST(NodeData, invoke_getter)
{
    TestNodeData data{42};
    NodeData* ptr = &data;

    auto res = ptr->invoke<double>(QStringLiteral("myDouble"));
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res.value(), 42);

    auto invalid = ptr->invoke<QString>(QStringLiteral("myDouble"));
    EXPECT_FALSE(invalid.has_value());
}

TEST(NodeData, invoke_getter_with_args)
{
    TestNodeData data{42};
    NodeData* ptr = &data;

    auto res = ptr->invoke<double>(QStringLiteral("myDoubleModified"),
                                   Q_ARG(int, 2), Q_ARG(QString, "test"));
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res.value(), 42 * 2 * 4);
}

/// check that QFileInfo can be recieved using invoke method
TEST(NodeData, invoke_getter_QFileInfo)
{
    auto data = NodeDataFactory::instance().makeData(typeId<FileData>());
    ASSERT_TRUE(data);

    auto res = data->invoke<QFileInfo>(QStringLiteral("value"));
    ASSERT_TRUE(res.has_value());
}

/// check that QFileInfo can be recieved using invoke method
TEST(NodeData, convert_same_type)
{
    auto doubleData = std::make_shared<DoubleData>(42);
    NodeDataPtr doubleDataPtr= doubleData;

    EXPECT_TRUE(NodeDataFactory::instance()
                    .canConvert(typeId<DoubleData>(), typeId<DoubleData>()));

    EXPECT_TRUE(intelli::convert(doubleData, typeId<DoubleData>()));
    EXPECT_TRUE(intelli::convert<DoubleData>(doubleData));

    EXPECT_TRUE(intelli::convert(doubleDataPtr, typeId<DoubleData>()));
    EXPECT_TRUE(intelli::convert<DoubleData>(doubleDataPtr));
}

TEST(NodeData, convert_incompatible_type)
{
    auto doubleData = std::make_shared<DoubleData>(42);
    NodeDataPtr doubleDataPtr= doubleData;

    EXPECT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));
    EXPECT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<DoubleData>(), typeId<TestNodeData>()));

    EXPECT_FALSE(intelli::convert(doubleData, typeId<TestNodeData>()));
    EXPECT_FALSE(intelli::convert<TestNodeData>(doubleData));

    EXPECT_FALSE(intelli::convert(doubleDataPtr, typeId<TestNodeData>()));
    EXPECT_FALSE(intelli::convert<TestNodeData>(doubleDataPtr));
}

TEST(NodeData, convert_compatible_type)
{
    auto doubleData = std::make_shared<DoubleData>(42);
    NodeDataPtr doubleDataPtr= doubleData;

    ASSERT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<DoubleData>(), typeId<TestNodeData>()));
    ASSERT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));

    EXPECT_FALSE(intelli::convert(doubleData, typeId<TestNodeData>()));
    EXPECT_FALSE(intelli::convert<TestNodeData>(doubleData));

    GT_INTELLI_REGISTER_INLINE_CONVERSION(DoubleData, TestNodeData, data->value())

    EXPECT_TRUE(NodeDataFactory::instance()
                     .canConvert(typeId<DoubleData>(), typeId<TestNodeData>()));
    EXPECT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));

    ASSERT_TRUE(intelli::convert(doubleData, typeId<TestNodeData>()));
    ASSERT_TRUE(intelli::convert<TestNodeData>(doubleData));
    ASSERT_TRUE(intelli::convert(doubleData, typeId<TestNodeData>()));
    ASSERT_TRUE(intelli::convert<TestNodeData>(doubleData));

    EXPECT_EQ(intelli::convert<TestNodeData>(doubleDataPtr)->myDouble(), doubleData->value());
}
