/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"
#include <intelli/memory.h>

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;

TEST(GraphUserVariables, set_and_remove)
{
    GraphUserVariables uv;

    ASSERT_TRUE(uv.empty());
    ASSERT_FALSE(uv.hasValue("test"));

    EXPECT_TRUE(uv.setValue("test", 42));
    EXPECT_TRUE(uv.hasValue("test"));
    EXPECT_EQ(uv.value("test"), QVariant::fromValue(42));

    EXPECT_EQ(uv.size(), 1);

    EXPECT_TRUE(uv.setValue("test_2", QString("Hello World")));
    EXPECT_TRUE(uv.hasValue("test_2"));
    EXPECT_EQ(uv.value("test_2"), QString("Hello World"));

    EXPECT_EQ(uv.size(), 2);

    EXPECT_TRUE(uv.setValue("test", 12));
    EXPECT_TRUE(uv.hasValue("test"));
    EXPECT_EQ(uv.value("test"), QVariant::fromValue(12));

    EXPECT_EQ(uv.size(), 2);
    EXPECT_EQ(uv.keys(), (QStringList{QString{"test"}, QString{"test_2"}}));

    EXPECT_TRUE(uv.remove("test"));
    EXPECT_FALSE(uv.hasValue("test"));
    EXPECT_EQ(uv.value("test"), QVariant{});

    EXPECT_EQ(uv.size(), 1);
    EXPECT_EQ(uv.keys(), (QStringList{QString{"test_2"}}));

    EXPECT_TRUE(uv.remove("test_2"));
    EXPECT_FALSE(uv.hasValue("test_2"));
    EXPECT_EQ(uv.value("test_2"), QVariant{});

    EXPECT_TRUE(uv.empty());
    EXPECT_EQ(uv.keys(), (QStringList{}));
}

TEST(GraphUserVariables, merge)
{
    GraphUserVariables uv1;
    uv1.setValue("a", 1);
    uv1.setValue("b", QString{"two"});
    uv1.setValue("c", 42.123);
    EXPECT_EQ(uv1.size(), 3);

    GraphUserVariables uv2;
    uv2.setValue("d", true);
    uv2.setValue("a", false);
    EXPECT_EQ(uv2.size(), 2);

    uv1.mergeWith(uv2);

    EXPECT_EQ(uv1.size(), 4);
    EXPECT_EQ(uv2.size(), 0);

    EXPECT_EQ(uv1.value("a"), QVariant::fromValue(false));
    EXPECT_EQ(uv1.value("b"), QVariant::fromValue(QString{"two"}));
    EXPECT_EQ(uv1.value("c"), QVariant::fromValue(42.123));
    EXPECT_EQ(uv1.value("d"), QVariant::fromValue(true));
}

TEST(GraphUserVariables, merge_graphs)
{
    Graph a;
    GraphUserVariables* uvA = a.findDirectChild<GraphUserVariables*>();
    ASSERT_TRUE(uvA);
    ASSERT_EQ(uvA, a.userVariables());

    auto bptr = std::make_unique<Graph>();
    Graph* b = bptr.get();
    GraphUserVariables* uvB = b->findDirectChild<GraphUserVariables*>();
    ASSERT_TRUE(uvB);
    ASSERT_EQ(uvB, b->userVariables());

    uvA->setValue("a", 1);
    uvA->setValue("b", QString{"two"});
    uvA->setValue("c", 42.123);
    EXPECT_EQ(uvA->size(), 3);

    uvB->setValue("d", true);
    uvB->setValue("a", false);
    EXPECT_EQ(uvB->size(), 2);

    GtObjectMemento mementoBefore = a.toMemento();

    b = a.appendNode(std::move(bptr));
    ASSERT_TRUE(b);

    GtObjectMemento mementoAfter = a.toMemento();
    GtObjectMementoDiff diff(mementoBefore, mementoAfter);

    // user variables objects should still be valid
    ASSERT_EQ(uvA, a.userVariables());
    ASSERT_EQ(uvB, b->userVariables());

    EXPECT_EQ(uvA->size(), 4);
    EXPECT_EQ(uvB->size(), 0);

    EXPECT_EQ(uvA->value("a"), QVariant::fromValue(false));
    EXPECT_EQ(uvA->value("b"), QVariant::fromValue(QString{"two"}));
    EXPECT_EQ(uvA->value("c"), QVariant::fromValue(42.123));
    EXPECT_EQ(uvA->value("d"), QVariant::fromValue(true));

    bool bDeleted = false;
    b->connect(b, &QObject::destroyed, b, [&](){
        bDeleted = true;
    });

    a.revertDiff(diff);
    b = nullptr;
    uvB = nullptr;

    EXPECT_TRUE(bDeleted);

    EXPECT_EQ(uvA->size(), 3);
    EXPECT_EQ(uvA->value("a"), QVariant::fromValue(1));
    EXPECT_EQ(uvA->value("b"), QVariant::fromValue(QString{"two"}));
    EXPECT_EQ(uvA->value("c"), QVariant::fromValue(42.123));
}
