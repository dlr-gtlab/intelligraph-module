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
}

TEST(NodeData, invoke_getter_invalid)
{
    TestNodeData data{42};
    NodeData* ptr = &data;

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

/// check that conversion for list types works as expected
TEST(NodeData, convert_list_type)
{
    EXPECT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<StringData>(),
                                 typeId<list<StringData>>()));

    bool registered = false;

    registered = GT_INTELLI_REGISTER_INLINE_CONVERSION(StringData, list<StringData>, QStringList{data->value()});

    ASSERT_TRUE(registered);

    EXPECT_TRUE(NodeDataFactory::instance()
                     .canConvert(typeId<StringData>(),
                                 typeId<list<StringData>>()));

    auto converted = convert<list<StringData>>(makeNodeData<StringData>("Hello World"));
    ASSERT_TRUE(converted);
    ASSERT_EQ(converted->iterate().size(), 1);
    ASSERT_EQ(converted->at(0), u"Hello World");
}

/// check that conversion for list types works as expected
TEST(NodeData, convert_invalid_data)
{
    EXPECT_TRUE(NodeDataFactory::instance()
                    .canConvert(typeId<list<IntData>>(), typeId<InvalidData>()));
    EXPECT_TRUE(NodeDataFactory::instance()
                    .canConvert(typeId<InvalidData>(), typeId<list<IntData>>()));

    // converting from or to invalid data always yields null
    ASSERT_FALSE(NodeDataFactory::instance()
                     .convert(makeNodeData<list<IntData>>(), typeId<InvalidData>()));
    ASSERT_FALSE(NodeDataFactory::instance()
                     .convert(makeNodeData<InvalidData>(), typeId<list<IntData>>()));
}

TEST(NodeData, list_types_type_ids)
{
    static_assert(is_list_type<ListData>::value,         "expected list type");
    static_assert(is_list_type<GenericListData>::value,  "expected list type");
    static_assert(is_list_type<list<StringData>>::value, "expected list type");
    static_assert(is_list_type<list<DoubleData>>::value, "expected list type");
    static_assert(is_list_type<StringListData>::value,   "expected list type");
    static_assert(!is_list_type<DoubleData>::value,      "expected non-list type");
    static_assert(!is_list_type<StringData>::value,      "expected non-list type");

    static_assert(std::is_same<intelli::unwrap_type_t<list<StringData>>, StringListData>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<intelli::unwrap_type_t<StringListData>, StringListData>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<intelli::unwrap_type_t<list<StringData>>, list_type_t<StringData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<intelli::unwrap_type_t<list<IntData>>, list_type_t<IntData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<intelli::unwrap_type_t<list<DoubleData>>, list_type_t<DoubleData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<intelli::unwrap_type_t<list<FileData>>, list_type_t<FileData>>::value,
                  "expect correct unwrapping");

    static_assert(std::is_same_v<inner_type_t<StringListData>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<StringData>>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<DoubleData>>, DoubleData>,
                  "expected double type");
    static_assert(std::is_same_v<inner_type_t<list<FileData>>, FileData>,
                  "expected file type");

    EXPECT_EQ(safeTypeId<ListData>(), QString{});
    EXPECT_EQ(safeTypeId<GenericListData>(), QString{});
}
/// check that conversion for incompatible types fails
TEST(NodeData, list_types)
{
    auto& factory = NodeDataFactory::instance();

    // double
    EXPECT_EQ(typeId<list<DoubleData>>(), listTypeId<DoubleData>());
    EXPECT_TRUE(factory.isListType(typeId<list<DoubleData>>()));
    EXPECT_FALSE(factory.isListType(typeId<DoubleData>()));
    EXPECT_TRUE(factory.hasListType(typeId<DoubleData>()));
    EXPECT_FALSE(factory.hasListType(typeId<list<DoubleData>>()));
    EXPECT_EQ(factory.innerType(typeId<list<DoubleData>>()), typeId<DoubleData>());
    EXPECT_TRUE(factory.innerType(typeId<DoubleData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<DoubleData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<DoubleData>()), typeId<list<DoubleData>>());

    // string
    EXPECT_EQ(typeId<list<StringData>>(), listTypeId<StringData>());
    EXPECT_EQ(typeId<list<StringData>>(), typeId<StringListData>());
    EXPECT_TRUE(factory.isListType(typeId<list<StringData>>()));
    EXPECT_FALSE(factory.isListType(typeId<StringData>()));
    EXPECT_TRUE(factory.hasListType(typeId<StringData>()));
    EXPECT_FALSE(factory.hasListType(typeId<list<StringData>>()));
    EXPECT_EQ(factory.innerType(typeId<list<StringData>>()), typeId<StringData>());
    EXPECT_TRUE(factory.innerType(typeId<StringData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<StringData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<StringData>()), typeId<list<StringData>>());

    // file
    EXPECT_EQ(typeId<list<FileData>>(), listTypeId<FileData>());
    EXPECT_TRUE(factory.isListType(typeId<list<FileData>>()));
    EXPECT_FALSE(factory.isListType(typeId<FileData>()));
    EXPECT_EQ(factory.innerType(typeId<list<FileData>>()), typeId<FileData>());
    EXPECT_TRUE(factory.hasListType(typeId<FileData>()));
    EXPECT_FALSE(factory.hasListType(typeId<list<FileData>>()));
    EXPECT_TRUE(factory.innerType(typeId<FileData>()).isEmpty());
    EXPECT_TRUE(factory.listType(typeId<list<FileData>>()).isEmpty());
    EXPECT_EQ(factory.listType(typeId<FileData>()), typeId<list<FileData>>());

}

TEST(NodeData, list_types_invalid_data)
{
    auto& factory = NodeDataFactory::instance();

    // cannot use typeId<list<InvalidData>>() -> produces compiler error
    // cannot use listTypeId<InvalidData>()   -> produces compiler error
    EXPECT_FALSE(factory.isListType(u"#list#intelli::InvalidData"));
    EXPECT_FALSE(factory.isListType(typeId<InvalidData>()));
    EXPECT_EQ(factory.innerType(u"#list#intelli::InvalidData"), QString{});
    EXPECT_TRUE(factory.innerType(typeId<InvalidData>()).isEmpty());
    EXPECT_TRUE(factory.listType(u"#list#intelli::InvalidData").isEmpty());
    EXPECT_TRUE(factory.listType(typeId<InvalidData>()).isEmpty());
}

TEST(NodeData, list_types_stringlist_backwards_compat)
{
    auto& factory = NodeDataFactory::instance();

    // backwards compatibility for stringlist
    EXPECT_TRUE(factory.isListType(QString{GT_CLASSNAME(StringListData)}));
    EXPECT_FALSE(factory.hasListType(QString{GT_CLASSNAME(StringListData)}));
    EXPECT_EQ(factory.innerType(QString{GT_CLASSNAME(StringListData)}), typeId<StringData>());
    EXPECT_TRUE(factory.listType(QString{GT_CLASSNAME(StringListData)}).isEmpty());
    EXPECT_TRUE(factory.listType(QString{GT_CLASSNAME(StringListData)}).isEmpty());
}

TEST(NodeData, is_known_type)
{
    auto& factory = NodeDataFactory::instance();

    EXPECT_TRUE(factory.isKnownType(typeId<list<DoubleData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<DoubleData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<list<StringData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<StringData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<list<FileData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<FileData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<InvalidData>()));
    EXPECT_FALSE(factory.isKnownType(u"#list#intelli::InvalidData"));
}

TEST(NodeData, is_known_type_stringlist_backwards_compat)
{
    auto& factory = NodeDataFactory::instance();

    EXPECT_TRUE(factory.isKnownType(QString{GT_CLASSNAME(StringListData)}));
}

TEST(NodeData, make_data)
{
    auto& factory = NodeDataFactory::instance();

    auto data = factory.makeData(typeId<DoubleData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(DoubleData));
    data = factory.makeData(typeId<list<DoubleData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));
    data = factory.makeListData(typeId<DoubleData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));
    data = factory.makeListData(typeId<list<DoubleData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));

    data = factory.makeData(typeId<StringData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringData));
    data = factory.makeData(typeId<list<StringData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
    data = factory.makeListData(typeId<StringData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
    data = factory.makeListData(typeId<list<StringData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));

    data = factory.makeData(typeId<FileData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(FileData));
    data = factory.makeData(typeId<list<FileData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));
    data = factory.makeListData(typeId<FileData>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));
    data = factory.makeListData(typeId<list<FileData>>());
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(GenericListData));
}

TEST(NodeData, make_data_stringlist_backwards_compat)
{
    auto& factory = NodeDataFactory::instance();

    auto data = factory.makeData(GT_CLASSNAME(StringListData));
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
    data = factory.makeListData(GT_CLASSNAME(StringListData));
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
}

TEST(NodeData, make_data_invalid_data)
{
    auto& factory = NodeDataFactory::instance();

    auto data = factory.makeData(GT_CLASSNAME(InvalidData));
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(InvalidData));
    data = factory.makeData("#list#intelli::InvalidData");
    EXPECT_FALSE(data);
    data = factory.makeListData(typeId<InvalidData>());
    EXPECT_FALSE(data);
    data = factory.makeListData("#list#intelli::InvalidData");
    EXPECT_FALSE(data);
}
