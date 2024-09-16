/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef TESTDYNAMICNODE_H
#define TESTDYNAMICNODE_H

#include <intelli/dynamicnode.h>

class TestDynamicNode : public intelli::DynamicNode
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestDynamicNode();
};

class TestDynamicWhiteListNode : public intelli::DynamicNode
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestDynamicWhiteListNode(QStringList inputWhiteList = {},
                                         QStringList outputWhiteList = {});
};

#endif // TESTDYNAMICNODE_H
