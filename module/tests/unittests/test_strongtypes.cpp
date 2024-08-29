#include <gtest/gtest.h>

#include <intelli/globals.h>

TEST(StrongTypes, initial_value)
{
    using namespace intelli;

    StrongType<int, struct Bla_, 42> st;
    EXPECT_EQ(st, 42);
}

TEST(StrongTypes, default_constructed_type_is_invalid)
{
    using namespace intelli;

    PortId id;
    EXPECT_EQ(id, invalid<PortId>());
}

TEST(StrongTypes, object_stores_correct_value)
{
    using namespace intelli;

    PortId id{42};

    EXPECT_EQ(id, 42);
    EXPECT_EQ(id.value(), 42);

    id = PortId{12};
    EXPECT_EQ(id, 12);
}

TEST(StrongTypes, compare_different_types)
{
    using namespace intelli;

    PortId id{42};
    PortIndex idx{42};
    NodeId nodeId{12};

    EXPECT_EQ(id, idx.value());
    EXPECT_NE(id, nodeId.value());
}

TEST(StrongTypes, compare_equal)
{
    using namespace intelli;

    PortId id{42};

    EXPECT_EQ(id, id);
}

TEST(StrongTypes, compare_not_equal)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_NE(id1, id2);
}

TEST(StrongTypes, compare_greater_than)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_GT(id1, id2);
    EXPECT_GE(id1, id1);
}

TEST(StrongTypes, compare_less_than)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_LT(id2, id1);
    EXPECT_LE(id2, id2);
}

TEST(StrongTypes, add)
{
    using namespace intelli;

    PortId id1{42}, id2{12};
    id1 += id2;
    EXPECT_EQ(id1, 54);
}

TEST(StrongTypes, subtract)
{
    using namespace intelli;

    PortId id1{42}, id2{12};
    id1 -= id2;
    EXPECT_EQ(id1, 30);
}

TEST(StrongTypes, multiply)
{
    using namespace intelli;

    PortId id1{10}, id2{12};
    id1 *= id2;
    EXPECT_EQ(id1, 120);
}

TEST(StrongTypes, division)
{
    using namespace intelli;

    PortId id1{120}, id2{12};
    id1 /= id2;
    EXPECT_EQ(id1, 10);
}

TEST(StrongTypes, increment)
{
    using namespace intelli;

    PortId id{1};
    id++;
    EXPECT_EQ(id, 2);
    ++id;
    EXPECT_EQ(id, 3);

    EXPECT_EQ(id++, 3);
    EXPECT_EQ(id, 4);
}

TEST(StrongTypes, decrement)
{
    using namespace intelli;

    PortId id{5};
    id--;
    EXPECT_EQ(id, 4);
    --id;
    EXPECT_EQ(id, 3);

    EXPECT_EQ(id--, 3);
    EXPECT_EQ(id, 2);
}
