/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 8.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
