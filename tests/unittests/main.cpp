/*
 * GTlab Geometry Module
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 *
 * Author: Martin Siggel <martin.siggel@dlr.de>
 */

#include "gtest/gtest.h"

#include "node/test_dynamic.h"

#include "intelli/graph.h"
#include "intelli/connection.h"
#include <intelli/core.h>

#include <gt_objectfactory.h>

#include <gt_logging.h>

#include <QCoreApplication>

#include <QtConcurrent>

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

        gtObjectFactory->registerClass(intelli::Graph::staticMetaObject);
        gtObjectFactory->registerClass(intelli::Connection::staticMetaObject);
    }();

    QCoreApplication a(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    intelli::initModule();

    bool success = RUN_ALL_TESTS();

    return success;
}
