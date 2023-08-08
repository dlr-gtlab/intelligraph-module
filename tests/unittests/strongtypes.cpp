#include <gtest/gtest.h>

#include "gt_igglobals.h"

TEST(StrogTypes, default_constructed_type_is_invalid)
{
    using namespace gt::ig;

    PortId id;
    EXPECT_EQ(id, gt::ig::invalid<PortId>());
}
