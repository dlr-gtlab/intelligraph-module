#
# GTlab Geometry Module
# SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
#
# Author: Martin Siggel <martin.siggel@dlr.de>
#

cmake_minimum_required(VERSION 3.15)
project(IntelliGraph-Unittests)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (NOT TARGET gtest)
    include(AddGoogleTest)
endif()

add_definitions(-DGTlabIntelliGraphTest)

add_executable(IntelliGraphUnitTests
    test_helper.h

    main.cpp
    graph.cpp
    graphbuilder.cpp
    graphexecmodel.cpp
    portdata.cpp
    strongtypes.cpp

    node/test_dynamic.h node/test_dynamic.cpp
    node/test_node.h node/test_node.cpp
)

target_link_libraries(IntelliGraphUnitTests PRIVATE
    gtest
    GTlabIntelliGraph
    Qt5::Concurrent
    QtNodes::QtNodes
)
target_compile_definitions(IntelliGraphUnitTests PRIVATE
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
)

add_test(NAME Unittests COMMAND IntelliGraphUnitTests)
