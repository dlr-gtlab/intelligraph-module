/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "test_helper.h"

#include <intelli/node/propertyinput/boolinputnode.h>
#include <intelli/node/propertyinput/intinputnode.h>
#include <intelli/node/propertyinput/doubleinputnode.h>

using namespace intelli;
TEST(BoolInputNode, accessPropertyAndReadValue)
{
    BoolInputNode n;

    GtAbstractProperty& prop =  n.getProperty();

    QVariant trueVariant = {true};
    prop.setValueFromVariant(trueVariant);

    ASSERT_TRUE(n.value());

    QVariant falseVariant = {false};
    prop.setValueFromVariant(falseVariant);

    ASSERT_FALSE(n.value());
}

TEST(IntInputNode, accessPropertyAndReadValue)
{
    IntInputNode n;

    GtAbstractProperty& prop =  n.getProperty();

    QVariant variant13 = {13};
    prop.setValueFromVariant(variant13);

    ASSERT_EQ(n.value(), 13);

    QVariant variantMinus7 = {-7};
    prop.setValueFromVariant(variantMinus7);

    ASSERT_EQ(n.value(), -7);
}

TEST(DoubleInputNode, accessPropertyAndReadValue)
{
    DoubleInputNode n;

    GtAbstractProperty& prop =  n.getProperty();

    QVariant var1 = {-123.1586};
    prop.setValueFromVariant(var1);

    ASSERT_EQ(n.value(), -123.1586);

    QVariant var2 = {123.2e13};
    prop.setValueFromVariant(var2);

    ASSERT_EQ(n.value(), 123.2e13);
}
