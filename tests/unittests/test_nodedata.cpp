/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 16.2.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "test_helper.h"

#include "data/test_nodedata.h"

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
