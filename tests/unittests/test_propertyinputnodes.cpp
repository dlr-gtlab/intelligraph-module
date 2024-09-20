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
#include <intelli/node/propertyinput/stringinputnode.h>
#include <intelli/node/propertyinput/objectinputnode.h>

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

TEST(StringInputNode, accessPropertyAndReadValue)
{
    StringInputNode n;

    GtAbstractProperty& prop =  n.getProperty();

    QVariant var1 = {"Hello World"};
    prop.setValueFromVariant(var1);

    ASSERT_STREQ(n.value().toStdString().c_str(), "Hello World");

    QVariant var2 = {"Testing is important"};
    prop.setValueFromVariant(var2);

    ASSERT_STREQ(n.value().toStdString().c_str(), "Testing is important");
}

TEST(ObjectInputNode, accessPropertyAndReadValue)
{
    ObjectInputNode n;
    GtAbstractProperty& prop =  n.getProperty();

    /// initialy linked object is not existing
    ASSERT_TRUE(n.linkedObject() == nullptr);

    auto* root = new GtObject;
    auto* child = new GtObject;
    root->appendChild(child);

    prop.setValueFromVariant({child->uuid()});

    /// root object is not set. Object will not be found
    ASSERT_TRUE(n.linkedObject() == nullptr);

    /// if root is used as root the object should be able to be found
    ASSERT_TRUE(n.linkedObject(root) == child);

    delete root;
}

