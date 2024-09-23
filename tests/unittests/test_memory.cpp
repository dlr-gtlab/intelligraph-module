/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/memory.h>

#include "test_helper.h"

#include "gt_eventloop.h"

TEST(Memory, volatile_ptr_delete_later)
{
    using namespace intelli;

    QPointer<QObject> p;

    {
        ASSERT_FALSE(p);

        auto obj = intelli::make_volatile<QObject, DeferredDeleter>();
        p = obj;

        ASSERT_TRUE(p);
    }

    EXPECT_TRUE(p);

    // do one iteration
    GtEventLoop eventLoop(std::chrono::milliseconds(1));
    eventLoop.exec();

    EXPECT_FALSE(p);
}

TEST(Memory, volatile_ptr_delete_now)
{
    using namespace intelli;

    QPointer<QObject> p;

    {
        ASSERT_FALSE(p);

        auto obj = intelli::make_volatile<QObject, DirectDeleter>();
        p = obj;

        ASSERT_TRUE(p);
    }

    EXPECT_FALSE(p);
}
