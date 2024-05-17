/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 13.5.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#include <gtest/gtest.h>

#include <intelli/globals.h>


TEST(Globals, quantize)
{
    using namespace intelli;

    EXPECT_EQ(quantize(QPointF(42.4,  9.75), 5), QPoint(40,  10));
    EXPECT_EQ(quantize(QPointF(42.7, -9.75), 5), QPoint(40, -10));
    EXPECT_EQ(quantize(QPointF(43.0, -9.75), 5), QPoint(45, -10));
}
