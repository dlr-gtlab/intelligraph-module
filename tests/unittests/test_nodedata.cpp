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
#include <intelli/data/list.h>
#include <intelli/data/string.h>
#include <intelli/data/stringlist.h>
#include <intelli/data/double.h>
#include <intelli/data/invalid.h>

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

/// check that conversion for the same types is supported
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

/// check that conversion for incompatible types fails
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

/// check that conversion for compatible types succeeds
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

/// check that conversion for incompatible types fails
TEST(NodeData, list_types)
{
    static_assert(is_list_type<BaseListData>::value,     "expected list type");
    static_assert(is_list_type<ListData>::value,         "expected list type");
    static_assert(is_list_type<list<StringData>>::value, "expected list type");
    static_assert(is_list_type<list<DoubleData>>::value, "expected list type");
    static_assert(is_list_type<StringListData>::value,   "expected list type");
    static_assert(!is_list_type<DoubleData>::value,      "expected non-list type");
    static_assert(!is_list_type<StringData>::value,      "expected non-list type");

    static_assert(std::is_same_v<inner_type_t<StringListData>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<StringData>>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<DoubleData>>, DoubleData>,
                  "expected double type");
    static_assert(std::is_same_v<inner_type_t<list<FileData>>, FileData>,
                  "expected file type");

    auto& factory = NodeDataFactory::instance();
    // double
    EXPECT_EQ(typeId<list<DoubleData>>(), listTypeId<DoubleData>());
    EXPECT_TRUE(factory.isListType(typeId<list<DoubleData>>()));
    EXPECT_FALSE(factory.isListType(typeId<DoubleData>()));
    EXPECT_EQ(factory.innerType(typeId<list<DoubleData>>()), typeId<DoubleData>());
    EXPECT_TRUE(factory.innerType(typeId<DoubleData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<DoubleData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<DoubleData>()), typeId<list<DoubleData>>());

    // string
    EXPECT_EQ(typeId<list<StringData>>(), listTypeId<StringData>());
    EXPECT_EQ(typeId<list<StringData>>(), typeId<StringListData>());
    EXPECT_TRUE(factory.isListType(typeId<list<StringData>>()));
    EXPECT_FALSE(factory.isListType(typeId<StringData>()));
    EXPECT_EQ(factory.innerType(typeId<list<StringData>>()), typeId<StringData>());
    EXPECT_EQ(factory.innerType(typeId<StringListData>()), typeId<StringData>());
    EXPECT_TRUE(factory.innerType(typeId<StringData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<StringData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<StringData>()), typeId<list<StringData>>());

    // file
    EXPECT_EQ(typeId<list<FileData>>(), listTypeId<FileData>());
    EXPECT_TRUE(factory.isListType(typeId<list<FileData>>()));
    EXPECT_FALSE(factory.isListType(typeId<FileData>()));
    EXPECT_EQ(factory.innerType(typeId<list<FileData>>()), typeId<FileData>());
    EXPECT_TRUE(factory.innerType(typeId<FileData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<FileData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<FileData>()), typeId<list<FileData>>());

    // invalid node data
    // cannot use typeId<list<InvalidData>>() -> produces compiler error
    // cannot use listTypeId<InvalidData>()   -> produces compiler error
    EXPECT_TRUE(factory.isListType(u"#list#intelli::InvalidData"));
    EXPECT_FALSE(factory.isListType(typeId<InvalidData>()));
    EXPECT_EQ(factory.innerType(u"#list#intelli::InvalidData"), typeId<InvalidData>());
    EXPECT_TRUE(factory.innerType(typeId<InvalidData>()).isEmpty());
    EXPECT_TRUE(factory.listType(u"#list#intelli::InvalidData").isEmpty());
    EXPECT_TRUE(factory.listType(typeId<InvalidData>()).isEmpty());
}
