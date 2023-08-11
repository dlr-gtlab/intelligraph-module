#include <gtest/gtest.h>

#include <intelli/globals.h>

TEST(StrongTypes, default_constructed_type_is_invalid)
{
    using namespace intelli;

    PortId id;
    EXPECT_EQ(id, invalid<PortId>());
}
