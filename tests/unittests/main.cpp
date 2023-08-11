/*
 * GTlab Geometry Module
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 *
 * Author: Martin Siggel <martin.siggel@dlr.de>
 */

#include "gtest/gtest.h"

#include <gt_logging.h>

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
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
