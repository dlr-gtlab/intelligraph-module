/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "gtest/gtest.h"

#include "node/test_dynamic.h"
#include "node/test_node.h"
#include "data/test_nodedata.h"

#include <intelli/connection.h>
#include <intelli/core.h>
#include <intelli/nodefactory.h>

#include <gt_objectfactory.h>

#include <gt_logging.h>

#include <QCoreApplication>

auto init_log_once = [](){
    auto& logger = gt::log::Logger::instance();
    logger.addDestination("console", gt::log::makeDebugOutputDestination());
    logger.setLoggingLevel(gt::log::TraceLevel);
    logger.setVerbosity(gt::log::Everything);
    return 0;
}();

int
main(int argc, char** argv)
{
    [](){
        TestDynamicNode::registerOnce();
        TestDynamicWhiteListNode::registerOnce();
        TestNode::registerOnce();
        TestSleepyNode::registerOnce();
        TestNumberInputNode::registerOnce();
        TestNodeData::registerOnce();
        gtObjectFactory->registerClass(intelli::Connection::staticMetaObject);
    }();

    QCoreApplication a(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    intelli::initModule();

    bool success = RUN_ALL_TESTS();

    return success;
}
