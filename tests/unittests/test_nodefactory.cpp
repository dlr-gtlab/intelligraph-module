/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"
#include "node/test_node.h"
#include "intelli/nodefactory.h"

#include "intelli/node/input/boolinput.h"
#include "intelli/node/input/intinput.h"
#include "intelli/node/input/doubleinput.h"
#include "intelli/node/input/stringinput.h"

#include <gt_object.h>
#include <gt_objectfactory.h>

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{

class TestNodeFactory : public intelli::NodeFactory
{
public:
    TestNodeFactory() {}
};

} // namespace

TEST(NodeFactory, initialization)
{
    TestNodeFactory factory;

    EXPECT_EQ(factory.registeredNodes().size(), 0);
    EXPECT_EQ(factory.registeredCategories().size(), 0);
    EXPECT_EQ(factory.sortedCategories().size(), 0);

    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(intelli::BoolInputNode)));
    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(intelli::IntInputNode)));
    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(intelli::DoubleInputNode)));
    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(intelli::StringInputNode)));
    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(TestUnregisteredNode)));
    EXPECT_FALSE(factory.isKnownNode(GT_CLASSNAME(TestDeprecatedNode)));
}

TEST(NodeFactory, category_order_priority)
{
    TestNodeFactory factory;

    // preregistered
    EXPECT_EQ(factory.categoryOrderPriority("[DEPRECATED]"),
              intelli::CategoryOrderPriority::Last + 1);
    EXPECT_EQ(factory.categoryOrderPriority("DOES NOT EXIST"), 0);

    // category order are registered even if the category was not yet registered
    factory.setCategoryOrderPriority("CAT_A", 10);
    EXPECT_EQ(factory.categoryOrderPriority("CAT_A"), 10);
    // does not register the category
    EXPECT_EQ(factory.registeredCategories().size(), 0);

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_A", "CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::IntInputNode>(QString{"CAT_B"}));

    EXPECT_EQ(factory.categoryOrderPriority("CAT_A"), 10);
    EXPECT_EQ(factory.categoryOrderPriority("CAT_B"), 0);
    EXPECT_EQ(factory.categoryOrderPriority("CAT_C"), 0);

    factory.setCategoryOrderPriority("CAT_C", -10);

    EXPECT_EQ(factory.categoryOrderPriority("CAT_A"), 10);
    EXPECT_EQ(factory.categoryOrderPriority("CAT_B"), 0);
    EXPECT_EQ(factory.categoryOrderPriority("CAT_C"), -10);

    EXPECT_EQ(factory.sortedCategories(),
              (QStringList{"CAT_B", "CAT_A"}));
}

TEST(NodeFactory, category_sorting)
{
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_A", "CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::IntInputNode>(intelli::View<QString>{"CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::DoubleInputNode>(QString{"CAT_C"}));

    EXPECT_EQ(factory.registeredCategories().size(), 3);
    EXPECT_EQ(factory.sortedCategories().size(), 3);
    EXPECT_EQ(factory.sortedCategories(),
              (QStringList{"CAT_A", "CAT_B", "CAT_C"}));

    EXPECT_TRUE(factory.registerNode<intelli::StringInputNode>(QString{}));
    EXPECT_EQ(factory.registeredCategories().size(), 3);
    EXPECT_EQ(factory.sortedCategories().size(), 3);
    EXPECT_EQ(factory.sortedCategories(),
              (QStringList{"CAT_A", "CAT_B", "CAT_C"}));

    factory.setCategoryOrderPriority("CAT_C", -10);
    factory.setCategoryOrderPriority("CAT_A", 10);
    EXPECT_EQ(factory.sortedCategories(),
              (QStringList{"CAT_C", "CAT_B", "CAT_A"}));

    EXPECT_TRUE(factory.registerNode<TestDeprecatedNode>("CAT_D"));
    EXPECT_EQ(factory.registeredCategories().size(), 4);
    EXPECT_EQ(factory.sortedCategories().size(), 4);
    EXPECT_EQ(factory.sortedCategories(),
              (QStringList{"CAT_C", "CAT_B", "CAT_A", "[DEPRECATED]"}));
}

TEST(NodeFactory, node_categories)
{
    TestNodeFactory factory;
    TestNodeFactory factory2;
    TestNodeFactory factory3;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_B", "CAT_A"}));
    EXPECT_TRUE(factory.registerNode<intelli::IntInputNode>(intelli::View<QString>{"CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::DoubleInputNode>(QString{"CAT_C"}));
    EXPECT_TRUE(factory.registerNode<TestDeprecatedNode>(QString{"CAT_D"}));
    EXPECT_TRUE(factory2.registerNode<TestDeprecatedNode>(intelli::View<QString>{}));
    EXPECT_TRUE(factory3.registerNode<TestDeprecatedNode>(QString{}));

    auto categories = factory.nodeCategories(GT_CLASSNAME(intelli::BoolInputNode));
    ASSERT_EQ(categories.size(), 2);
    EXPECT_EQ(categories.at(0), "CAT_A");
    EXPECT_EQ(categories.at(1), "CAT_B");
    categories = factory.nodeCategories(GT_CLASSNAME(intelli::IntInputNode));
    ASSERT_EQ(categories.size(), 1);
    EXPECT_EQ(categories.at(0), "CAT_B");
    categories = factory.nodeCategories(GT_CLASSNAME(intelli::DoubleInputNode));
    ASSERT_EQ(categories.size(), 1);
    EXPECT_EQ(categories.at(0), "CAT_C");
    categories = factory.nodeCategories(GT_CLASSNAME(TestDeprecatedNode));
    ASSERT_EQ(categories.size(), 1);
    EXPECT_EQ(categories.at(0), "[DEPRECATED]");
    categories = factory2.nodeCategories(GT_CLASSNAME(TestDeprecatedNode));
    ASSERT_EQ(categories.size(), 0);
    categories = factory3.nodeCategories(GT_CLASSNAME(TestDeprecatedNode));
    ASSERT_EQ(categories.size(), 0);
    categories = factory3.nodeCategories("UNKNOWN NODE");
    ASSERT_EQ(categories.size(), 0);
}

TEST(NodeFactory, node_categories_with_duplicates)
{
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_A", "CAT_B", "CAT_A"}));
    EXPECT_TRUE(factory.registerNode<intelli::DoubleInputNode>({"", "CAT_C"}));
    EXPECT_TRUE(factory.registerNode<TestDeprecatedNode>({"CAT_B", "CAT_C"}));

    auto categories = factory.nodeCategories(GT_CLASSNAME(intelli::BoolInputNode));
    ASSERT_EQ(categories.size(), 2);
    EXPECT_EQ(categories.at(0), "CAT_A");
    EXPECT_EQ(categories.at(1), "CAT_B");
    categories = factory.nodeCategories(GT_CLASSNAME(intelli::DoubleInputNode));
    ASSERT_EQ(categories.size(), 1);
    EXPECT_EQ(categories.at(0), "CAT_C");
    categories = factory.nodeCategories(GT_CLASSNAME(TestDeprecatedNode));
    ASSERT_EQ(categories.size(), 1);
    EXPECT_EQ(categories.at(0), "[DEPRECATED]");
}

TEST(NodeFactory, module_id)
{
    ASSERT_NE(QString{GT_MODULENAME()}.size(), 0);

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_A", "CAT_B"}));
    EXPECT_TRUE(factory.registerNode(GT_METADATA(intelli::IntInputNode), QString{"CAT_B"}, GT_MODULENAME()));
    EXPECT_TRUE(factory.registerNode(GT_METADATA(intelli::StringInputNode), QString{"CAT_B"}, "TEST"));

    EXPECT_EQ(factory.moduleId(GT_CLASSNAME(intelli::BoolInputNode)), GT_MODULENAME());
    EXPECT_EQ(factory.moduleId(GT_CLASSNAME(intelli::IntInputNode)), GT_MODULENAME());
    EXPECT_EQ(factory.moduleId(GT_CLASSNAME(intelli::StringInputNode)), "TEST");
    EXPECT_TRUE(factory.moduleId("UNKNOWN NODE").isEmpty());
#endif
}

TEST(NodeFactory, register_node)
{
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_A", "CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::IntInputNode>(intelli::View<QString>{"CAT_B"}));
    EXPECT_TRUE(factory.registerNode<intelli::DoubleInputNode>(QString{"CAT_C"}));
    EXPECT_TRUE(factory.registerNode<intelli::StringInputNode>(QString{}));

    auto nodes = factory.registeredNodes();
    std::sort(nodes.begin(), nodes.end());

    EXPECT_EQ(nodes, (QStringList{GT_CLASSNAME(intelli::BoolInputNode),
                                  GT_CLASSNAME(intelli::DoubleInputNode),
                                  GT_CLASSNAME(intelli::IntInputNode),
                                  GT_CLASSNAME(intelli::StringInputNode)}));

    EXPECT_TRUE(factory.isKnownNode(GT_CLASSNAME(intelli::BoolInputNode)));
    EXPECT_TRUE(factory.isKnownNode(GT_CLASSNAME(intelli::IntInputNode)));
    EXPECT_TRUE(factory.isKnownNode(GT_CLASSNAME(intelli::DoubleInputNode)));
    EXPECT_TRUE(factory.isKnownNode(GT_CLASSNAME(intelli::StringInputNode)));
}

TEST(NodeFactory, make_node)
{
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<TestUnregisteredNode>(QString{}));
    auto node = factory.tryMakeNode(GT_CLASSNAME(TestUnregisteredNode));
    ASSERT_TRUE(node);
    EXPECT_STREQ(node->metaObject()->className(), GT_CLASSNAME(TestUnregisteredNode));
    EXPECT_EQ(node->modelName(), factory.nodeModelName(GT_CLASSNAME(TestUnregisteredNode)));

    EXPECT_TRUE(factory.registerNode<TestDeprecatedNode>(QString{}));
    node = factory.tryMakeNode(GT_CLASSNAME(TestDeprecatedNode));
    ASSERT_TRUE(node);
    EXPECT_STREQ(node->metaObject()->className(), GT_CLASSNAME(TestDeprecatedNode));
    EXPECT_EQ(node->modelName(), factory.nodeModelName(GT_CLASSNAME(TestDeprecatedNode)));
}

TEST(NodeFactory, make_node_invalid)
{
    TestNodeFactory factory;

    auto node = factory.tryMakeNode("UNKNOWN NODE");
    ASSERT_FALSE(node);
    EXPECT_TRUE(factory.nodeModelName("UNKNOWN NODE").isEmpty());
}

// tests the deprecated API
TEST(NodeFactory, deprecated_api)
{
    TestNodeFactory factory;

    EXPECT_TRUE(factory.registerNode<intelli::BoolInputNode>({"CAT_B", "CAT_A"}));
    EXPECT_TRUE(factory.registerNode<TestDeprecatedNode>(QString{"CAT_D"}));

    EXPECT_EQ(factory.nodeCategory(GT_CLASSNAME(intelli::BoolInputNode)), "CAT_A");
    EXPECT_EQ(factory.nodeCategory(GT_CLASSNAME(TestDeprecatedNode)), "[DEPRECATED]");
    EXPECT_TRUE(factory.nodeCategory("UNKNOWN NODE").isEmpty());

    EXPECT_NO_THROW(
        auto node = factory.makeNode(GT_CLASSNAME(intelli::BoolInputNode));
        ASSERT_TRUE(node);
    );
    EXPECT_THROW((void) factory.makeNode("UNKNOWN NODE"), std::logic_error);
}
