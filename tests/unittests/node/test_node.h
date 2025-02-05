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
#include <intelli/node/sleepy.h>
#include <intelli/node/input/doubleinput.h>

/**
 * @brief The TestNode class. Can be used to test nodes that fail to evaluate
 */
class TestNode : public intelli::Node
{
    Q_OBJECT

public:

    bool failEvaluation = false;

    static void registerOnce();

    Q_INVOKABLE TestNode();

    using Node::setNodeFlag;
    using Node::setNodeEvalMode;
    using Node::insertInPort;
    using Node::insertOutPort;
    using Node::addInPort;
    using Node::addOutPort;
    using Node::removePort;

protected:

    void eval();
};

/**
 * @brief The TestSleepyNode class. Can be used to alter execution flags
 * of the sleepy node.
 */
class TestSleepyNode : public intelli::SleepyNode
{
public:

    static void registerOnce();

    Q_INVOKABLE TestSleepyNode() = default;

    using Node::setFlag;
    using Node::setNodeEvalMode;
};

/**
 * @brief The TestNumberInputNode class. Behaves like a double input node but
 * is executed async (detached) which allows for better debugging of the
 * execution chain.
 */
class TestNumberInputNode : public intelli::DoubleInputNode
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestNumberInputNode();
};

#endif // TESTNODE_H
