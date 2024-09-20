/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef TESTNODE_H
#define TESTNODE_H

#include <intelli/node.h>

class TestNode : public intelli::Node
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestNode();

    using Node::insertInPort;
    using Node::insertOutPort;
    using Node::addInPort;
    using Node::addOutPort;
    using Node::removePort;
};

#endif // TESTNODE_H
