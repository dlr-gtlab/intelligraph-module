/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"

#include "data/testdata.h"

#include <intelli/nodedatafactory.h>
#include <intelli/data/file.h>
#include <intelli/data/list.h>
#include <intelli/data/string.h>
#include <intelli/data/stringlist.h>
#include <intelli/data/double.h>
#include <intelli/data/int.h>
#include <intelli/data/bytearray.h>
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

    EXPECT_TRUE(convert(doubleData, typeId<DoubleData>()));
    EXPECT_TRUE(convert<DoubleData>(doubleData));

    EXPECT_TRUE(convert(doubleDataPtr, typeId<DoubleData>()));
    EXPECT_TRUE(convert<DoubleData>(doubleDataPtr));
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

    EXPECT_FALSE(convert(doubleData, typeId<TestNodeData>()));
    EXPECT_FALSE(convert<TestNodeData>(doubleData));

    EXPECT_FALSE(convert(doubleDataPtr, typeId<TestNodeData>()));
    EXPECT_FALSE(convert<TestNodeData>(doubleDataPtr));
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

    EXPECT_FALSE(convert(doubleData, typeId<TestNodeData>()));
    EXPECT_FALSE(convert<TestNodeData>(doubleData));

    bool success =
        GT_INTELLI_REGISTER_INLINE_CONVERSION(DoubleData, TestNodeData, data->value());
    EXPECT_TRUE(success);

    // restore
    auto cleanup = gt::finally([](){
        NodeDataFactory::instance().unregisterConversion(typeId<DoubleData>(), typeId<TestNodeData>());
    });
    Q_UNUSED(cleanup);

    EXPECT_TRUE(NodeDataFactory::instance()
                    .canConvert(typeId<DoubleData>(), typeId<TestNodeData>()));
    EXPECT_FALSE(NodeDataFactory::instance()
                     .canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));

    ASSERT_TRUE(convert(doubleData, typeId<TestNodeData>()));
    ASSERT_TRUE(convert<TestNodeData>(doubleData));
    ASSERT_TRUE(convert(doubleData, typeId<TestNodeData>()));
    ASSERT_TRUE(convert<TestNodeData>(doubleData));

    EXPECT_EQ(convert<TestNodeData>(doubleDataPtr)->myDouble(), doubleData->value());

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
    ASSERT_FALSE(convert(makeNodeData<list<IntData>>(), typeId<InvalidData>()));
    ASSERT_FALSE(convert(makeNodeData<InvalidData>(),   typeId<list<IntData>>()));
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
    static_assert(!is_list_type<QString>::value,         "expected non-data type");
    static_assert(!is_list_type<int>::value,             "expected non-data type");

    static_assert(!is_scalar_type<ListData>::value,         "expected list type");
    static_assert(!is_scalar_type<GenericListData>::value,  "expected list type");
    static_assert(!is_scalar_type<list<StringData>>::value, "expected list type");
    static_assert(!is_scalar_type<list<DoubleData>>::value, "expected list type");
    static_assert(!is_scalar_type<StringListData>::value,   "expected list type");
    static_assert(!is_scalar_type<NodeData>::value,         "expected abstract type");
    static_assert(is_scalar_type<DoubleData>::value,        "expected non-list type");
    static_assert(is_scalar_type<StringData>::value,        "expected non-list type");
    static_assert(is_scalar_type<InvalidData>::value,       "expected non-list type");
    static_assert(!is_scalar_type<QString>::value,          "expected non-data type");
    static_assert(!is_scalar_type<int>::value,              "expected non-data type");

    static_assert(std::is_same<unwrap_type_t<list<StringData>>, StringListData>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<unwrap_type_t<StringListData>, StringListData>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<unwrap_type_t<list<StringData>>, list_type_t<StringData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<unwrap_type_t<list<IntData>>, list_type_t<IntData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<unwrap_type_t<list<DoubleData>>, list_type_t<DoubleData>>::value,
                  "expect correct unwrapping");
    static_assert(std::is_same<unwrap_type_t<list<FileData>>, list_type_t<FileData>>::value,
                  "expect correct unwrapping");

    static_assert(std::is_same_v<inner_type_t<StringListData>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<StringData>>, StringData>,
                  "expected string type");
    static_assert(std::is_same_v<inner_type_t<list<DoubleData>>, DoubleData>,
                  "expected double type");
    static_assert(std::is_same_v<inner_type_t<list<FileData>>, FileData>,
                  "expected file type");
}

/// intelli::safeTypeId always compiles and yields the typeId if it exists else
/// an empty string
TEST(NodeData, safe_type_id)
{
    // abstract classes
    EXPECT_EQ(safeTypeId<NodeData>(), QString{});
    EXPECT_EQ(safeTypeId<ListData>(), QString{});

    // invalid types
    EXPECT_EQ(safeTypeId<QString>(), QString{});
    EXPECT_EQ(safeTypeId<void>(), QString{});
    EXPECT_EQ(safeTypeId<int>(), QString{});

    // cannot interfer inner type
    EXPECT_EQ(safeTypeId<GenericListData>(), QString{});

    EXPECT_EQ(safeTypeId<IntData>(), typeId<IntData>());
    EXPECT_EQ(safeTypeId<DoubleData>(), typeId<DoubleData>());
    EXPECT_EQ(safeTypeId<InvalidData>(), typeId<InvalidData>());

    // specialized type ids
    EXPECT_EQ(safeTypeId<StringListData>(), typeId<list<StringData>>());
    EXPECT_EQ(safeTypeId<TestListData>(), typeId<list<TestNodeData>>());
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
    EXPECT_FALSE(factory.isListType("#list#intelli::InvalidData"));
    EXPECT_FALSE(factory.isListType(typeId<InvalidData>()));
    EXPECT_EQ(factory.innerType("#list#intelli::InvalidData"), QString{});
    EXPECT_TRUE(factory.innerType(typeId<InvalidData>()).isEmpty());
    EXPECT_TRUE(factory.listType("#list#intelli::InvalidData").isEmpty());
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

TEST(NodeDataFactory, is_known_type)
{
    auto& factory = NodeDataFactory::instance();

    EXPECT_TRUE(factory.isKnownType(typeId<list<DoubleData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<DoubleData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<list<StringData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<StringData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<list<FileData>>()));
    EXPECT_TRUE(factory.isKnownType(typeId<FileData>()));

    EXPECT_TRUE(factory.isKnownType(typeId<InvalidData>()));
    EXPECT_FALSE(factory.isKnownType("#list#intelli::InvalidData"));
}

TEST(NodeDataFactory, is_known_type_stringlist_backwards_compat)
{
    auto& factory = NodeDataFactory::instance();

    EXPECT_TRUE(factory.isKnownType(QString{GT_CLASSNAME(StringListData)}));
}

TEST(NodeDataFactory, make_data)
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

TEST(NodeDataFactory, make_data_stringlist_backwards_compat)
{
    auto& factory = NodeDataFactory::instance();

    auto data = factory.makeData(GT_CLASSNAME(StringListData));
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
    data = factory.makeListData(GT_CLASSNAME(StringListData));
    ASSERT_TRUE(data);
    EXPECT_TRUE(data->metaObject()->className() == GT_CLASSNAME(StringListData));
}

TEST(NodeDataFactory, make_data_invalid_data)
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

/// check that NodeData::typeId() returns the correct type id, also for lists
TEST(NodeData, type_id)
{
    // scalar type via base pointer (virtual dispatch)
    DoubleData d{42};
    NodeData* base = &d;
    EXPECT_EQ(base->typeId(), typeId<DoubleData>());

    // typed generic list reflects its associated list type id
    auto const doubleId = typeId<DoubleData>();
    GenericListData typed{doubleId};
    EXPECT_EQ(typed.typeId(), typeId<list<DoubleData>>());

    // untyped generic list has no type id yet
    GenericListData untyped;
    EXPECT_TRUE(untyped.typeId().isEmpty());

    // specialized list types
    StringListData sl;
    EXPECT_EQ(sl.typeId(), typeId<list<StringData>>());

    TestListData tl;
    EXPECT_EQ(tl.typeId(), typeId<list<TestNodeData>>());
}

/// check appending to a typed GenericListData
TEST(NodeData, generic_list_append)
{
    auto const doubleId = typeId<DoubleData>();
    GenericListData list(doubleId);

    EXPECT_TRUE(list.append(makeNodeData<DoubleData>(1.0)));
    EXPECT_TRUE(list.append(makeNodeData<IntData>(2))); // int -> double
    EXPECT_FALSE(list.append(makeNodeData<StringData>(QStringLiteral("x")))); // no conversion
    EXPECT_FALSE(list.append(nullptr));
    ASSERT_EQ(list.size(), 2);

    // read back through the common ListData interface
    ListData* base = &list;
    auto first = convert<DoubleData>(base->at(0));
    ASSERT_TRUE(first);
    EXPECT_DOUBLE_EQ(first->value(), 1.0);

    auto second = convert<DoubleData>(base->at(1));
    ASSERT_TRUE(second);
    EXPECT_DOUBLE_EQ(second->value(), 2.0);
}

/// check that an untyped GenericListData infers its type from the first element
TEST(NodeData, generic_list_append_infers_type)
{
    GenericListData list;
    EXPECT_TRUE(list.empty());

    EXPECT_TRUE(list.append(makeNodeData<IntData>(3)));
    EXPECT_EQ(list.typeId(), listTypeId<IntData>());

    EXPECT_TRUE(list.append(makeNodeData<DoubleData>(4.0))); // Double -> Int
    EXPECT_FALSE(list.append(makeNodeData<StringData>(QStringLiteral("x")))); // no conversion
    ASSERT_EQ(list.size(), 2);
}

/// check that brace initialization selects the correct GenericListData ctor
TEST(NodeData, generic_list_braced_init)
{
    // single type id must use the (TypeId) ctor, not an initializer_list<TypeId> ctor
    GenericListData fromId{typeId<DoubleData>()};
    EXPECT_TRUE(fromId.empty());
    EXPECT_EQ(fromId.typeId(), listTypeId<DoubleData>());

    // initializer list of same-typed node data ptrs
    GenericListData list{makeNodeData<IntData>(1), makeNodeData<IntData>(2)};
    EXPECT_EQ(list.typeId(), listTypeId<IntData>());
    ASSERT_EQ(list.size(), 2);

    auto first = convert<IntData>(list.at(0));
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), 1);

    auto second = convert<IntData>(list.at(1));
    ASSERT_TRUE(second);
    EXPECT_EQ(second->value(), 2);
}

/// check the GenericListData constructor taking a list-like wrapper type
TEST(NodeData, generic_list_from_container)
{
    static_assert(!std::is_constructible<GenericListData, QVector<Ptr<NodeData>>>::value,
                  "expected generic list to not accept abstract types");
    static_assert(!std::is_constructible<GenericListData, QVector<Ptr<ListData>>>::value,
                  "expected generic list to not accept abstract types");
    static_assert(!std::is_constructible<GenericListData, QVector<QString>>::value,
                  "expected generic list to not accept non data types");
    static_assert(std::is_constructible<GenericListData, QVector<Ptr<IntData>>>::value,
                  "expected generic list to accept data types");
    static_assert(std::is_constructible<GenericListData, QVector<Ptr<DoubleData>>>::value,
                  "expected generic list to accept data types");
    static_assert(std::is_constructible<GenericListData, QVector<Ptr<InvalidData>>>::value,
                  "expected generic list to accept data types");
    static_assert(!std::is_constructible<GenericListData, QVector<Ptr<StringListData>>>::value,
                  "expected generic list to not accept list types");

    QVector<Ptr<IntData>> wrapper;
    wrapper.push_back(makeNodeData<IntData>(1));
    wrapper.push_back(makeNodeData<IntData>(2));
    wrapper.push_back(makeNodeData<IntData>(3));
    wrapper.push_back(nullptr);

    // elements are appended via append(), i.e. converted and silently dropped on failure
    GenericListData list{wrapper};
    EXPECT_EQ(list.typeId(), listTypeId<IntData>());
    ASSERT_EQ(list.size(), 3);

    auto first = convert<IntData>(list.at(0));
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), 1);

    auto second = convert<IntData>(list.at(1));
    ASSERT_TRUE(second);
    EXPECT_EQ(second->value(), 2);

    auto third = convert<IntData>(list.at(2));
    ASSERT_TRUE(third);
    EXPECT_EQ(third->value(), 3);
}

/// check appending to a StringListData
TEST(NodeData, string_list_append)
{
    StringListData list;

    EXPECT_TRUE(list.append(makeNodeData<StringData>(QStringLiteral("a"))));
    EXPECT_TRUE(list.append(makeNodeData<ByteArrayData>(QByteArray("b"))));  // ByteArray -> String
    EXPECT_FALSE(list.append(makeNodeData<DoubleData>(1.0))); // no conversion
    EXPECT_FALSE(list.append((NodeDataPtr)nullptr));

    list.append(QStringLiteral("c")); // uses QString overload

    ASSERT_EQ(list.size(), 3);
    QStringList expected{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
    EXPECT_EQ(list.value(), expected);
}

/// check that a specialized list type is registered and behaves correctly
TEST(NodeData, specialized_list_type)
{
    auto& factory = NodeDataFactory::instance();

    ASSERT_TRUE(factory.hasListType(typeId<TestNodeData>()));

    auto list = factory.makeListData(typeId<TestNodeData>());
    ASSERT_TRUE(list);
    EXPECT_TRUE(list->metaObject()->className() == GT_CLASSNAME(TestListData));
    EXPECT_EQ(list->typeId(), listTypeId<TestNodeData>());

    ASSERT_TRUE(list->append(makeNodeData<TestNodeData>(42.0)));
    EXPECT_FALSE(list->append(makeNodeData<IntData>(1))); // no Int -> Test
    EXPECT_FALSE(list->append(nullptr));
    ASSERT_EQ(list->size(), 1u);

    auto item = convert<TestNodeData>(list->at(0));
    ASSERT_TRUE(item);
    EXPECT_DOUBLE_EQ(item->myDouble(), 42.0);
}

/// check that registering multiple conversions for the same type pair fails
TEST(NodeDataFactory, register_conversion_duplicate_fails)
{
    auto& factory = NodeDataFactory::instance();

    auto conv = [](NodeDataPtr const&) -> NodeDataPtr {
        return makeNodeData<TestNodeData>(0.0);
    };
    auto otherConv = [](NodeDataPtr const&) -> NodeDataPtr {
        return makeNodeData<TestNodeData>(1.0);
    };

    factory.unregisterConversion(typeId<IntData>(), typeId<TestNodeData>());

    EXPECT_TRUE(factory.registerConversion(typeId<IntData>(), typeId<TestNodeData>(), conv));
    EXPECT_FALSE(factory.registerConversion(typeId<IntData>(), typeId<TestNodeData>(), otherConv));

    factory.unregisterConversion(typeId<IntData>(), typeId<TestNodeData>());
    EXPECT_FALSE(factory.canConvert(typeId<IntData>(), typeId<TestNodeData>()));
}

/// check that registering conversions involving a generic list type works
TEST(NodeDataFactory, register_conversion_list_type)
{
    auto& factory = NodeDataFactory::instance();

    auto toList = [](NodeDataPtr const& data) -> NodeDataPtr {
        auto list = std::make_shared<GenericListData>(typeId<IntData>());
        list->append(data);
        return list;
    };

    TypeId const from = typeId<IntData>();
    TypeId const to   = typeId<list<IntData>>();

    factory.unregisterConversion(from, to);

    ASSERT_TRUE(factory.registerConversion(from, to, toList));
    EXPECT_TRUE(factory.canConvert(from, to));

    auto converted = factory.convert(makeNodeData<IntData>(42), to);
    ASSERT_TRUE(converted);

    auto listData = std::static_pointer_cast<const ListData>(converted);
    ASSERT_EQ(listData->size(), 1u);

    auto item = convert<IntData>(listData->at(0));
    ASSERT_TRUE(item);
    EXPECT_EQ(item->value(), 42);

    factory.unregisterConversion(from, to);
    EXPECT_FALSE(factory.canConvert(from, to));
}

/// check that the type based conversion helpers only work with specialized list types
TEST(NodeDataFactory, convert_generic_list_type_yields_null)
{
    auto& factory = NodeDataFactory::instance();

    auto toList = [](NodeDataPtr const& data) -> NodeDataPtr {
        auto list = std::make_shared<GenericListData>(typeId<IntData>());
        list->append(data);
        return list;
    };

    TypeId const from = typeId<IntData>();
    TypeId const to   = typeId<list<IntData>>();

    factory.unregisterConversion(from, to);
    ASSERT_TRUE(factory.registerConversion(from, to, toList));

    // string based conversion works for generic lists...
    auto converted = convert(makeNodeData<IntData>(42), to);
    ASSERT_TRUE(converted);
    EXPECT_EQ(std::static_pointer_cast<const ListData>(converted)->size(), 1u);

    // ...but the type based helper yields null, even though a conversion exists
    // TODO fix
    EXPECT_TRUE(convert<list<IntData>>(makeNodeData<IntData>(42)));

    // type based conversion can still cast an existing generic list
    auto casted = convert<list<IntData>>(converted);
    ASSERT_TRUE(casted);
    EXPECT_EQ(casted->size(), 1);

    factory.unregisterConversion(from, to);

    // type based conversion works for specialized list types
    GT_INTELLI_REGISTER_INLINE_CONVERSION(StringData, list<StringData>, QStringList{data->value()})

    auto strings = convert<list<StringData>>(makeNodeData<StringData>(QStringLiteral("Hello World")));
    ASSERT_TRUE(strings);
    ASSERT_EQ(strings->size(), 1);
    EXPECT_EQ(strings->at(0), QStringLiteral("Hello World"));

    factory.unregisterConversion(typeId<StringData>(), typeId<list<StringData>>());
}

/// check that unregistering a single and all conversions works
TEST(NodeDataFactory, unregister_conversion)
{
    auto& factory = NodeDataFactory::instance();

    auto toTest = [](NodeDataPtr const&) -> NodeDataPtr {
        return makeNodeData<TestNodeData>(0.0);
    };

    factory.unregisterConversion(typeId<TestNodeData>(), typeId<IntData>());
    factory.unregisterConversion(typeId<TestNodeData>(), typeId<DoubleData>());

    ASSERT_TRUE(factory.registerConversion(typeId<TestNodeData>(), typeId<IntData>(), toTest));
    ASSERT_TRUE(factory.registerConversion(typeId<TestNodeData>(), typeId<DoubleData>(), toTest));

    // remove a single conversion
    factory.unregisterConversion(typeId<TestNodeData>(), typeId<IntData>());
    EXPECT_FALSE(factory.canConvert(typeId<TestNodeData>(), typeId<IntData>()));
    EXPECT_TRUE(factory.canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));

    // remove all conversions involving TestNodeData
    factory.unregisterConversions(typeId<TestNodeData>());
    EXPECT_FALSE(factory.canConvert(typeId<TestNodeData>(), typeId<DoubleData>()));

    // restore the InvalidData conversions removed by the bulk unregister
    auto toInvalid = [](NodeDataPtr const&) -> NodeDataPtr { return nullptr; };
    factory.registerConversion(typeId<TestNodeData>(), typeId<InvalidData>(), toInvalid);
    factory.registerConversion(typeId<InvalidData>(), typeId<TestNodeData>(), toInvalid);
}

/// check that unregistering data (and its list type/conversions) works
TEST(NodeDataFactory, unregister_data)
{
    auto& factory = NodeDataFactory::instance();

    ASSERT_TRUE(factory.isKnownType(typeId<TestNodeData>()));
    ASSERT_TRUE(factory.hasListType(typeId<TestNodeData>()));
    ASSERT_TRUE(factory.isKnownType(typeId<list<TestNodeData>>()));
    ASSERT_TRUE(factory.canConvert(typeId<TestNodeData>(), typeId<InvalidData>()));

    factory.unregisterData(TestNodeData::staticMetaObject);

    EXPECT_FALSE(factory.isKnownType(typeId<TestNodeData>()));
    EXPECT_FALSE(factory.hasListType(typeId<TestNodeData>()));
    EXPECT_FALSE(factory.isKnownType(typeId<list<TestNodeData>>()));
    EXPECT_FALSE(factory.canConvert(typeId<TestNodeData>(), typeId<InvalidData>()));

    // re-register to restore state for the other tests
    ASSERT_TRUE(factory.registerData<TestNodeData>());
    EXPECT_TRUE(factory.isKnownType(typeId<TestNodeData>()));
    EXPECT_TRUE(factory.hasListType(typeId<TestNodeData>()));
    ASSERT_TRUE(factory.isKnownType(typeId<list<TestNodeData>>()));
    ASSERT_TRUE(factory.canConvert(typeId<TestNodeData>(), typeId<InvalidData>()));
}
