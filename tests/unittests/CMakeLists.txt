# GTlab IntelliGraph
#
#  SPDX-License-Identifier: BSD-3-Clause
#  SPDX-FileCopyrightText: 2024 German Aerospace Center

cmake_minimum_required(VERSION 3.15)
project(IntelliGraph-Unittests)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (NOT TARGET gtest)
    include(AddGoogleTest)
endif()

add_library(GraphSelectionActionTestPlugin MODULE
    test_graphselectionactionplugin.cpp
)
set_target_properties(GraphSelectionActionTestPlugin PROPERTIES AUTOMOC OFF)
target_link_libraries(GraphSelectionActionTestPlugin PRIVATE GTlabIntelliGraph)

add_definitions(-DGTlabIntelliGraphTest)

add_executable(IntelliGraphUnitTests
    main.cpp
    test_helper.h

    test_dynamicnode.cpp
    test_globals.cpp
    test_graph.cpp
    test_graphbuilder.cpp
    test_graphexecmodel.cpp
    test_graphselectionaction.cpp
    test_graphuservariables.cpp
    test_gui.cpp
    test_memory.cpp
    test_nodedata.cpp
    test_portinfo.cpp
    test_strongtypes.cpp

    data/test_nodedata.h data/test_nodedata.cpp

    node/test_dynamic.h node/test_dynamic.cpp
    node/test_node.h node/test_node.cpp
)

target_link_libraries(IntelliGraphUnitTests PRIVATE
    gtest
    GTlabIntelliGraph
    Qt${QT_VERSION_MAJOR}::Concurrent
)
target_compile_definitions(IntelliGraphUnitTests PRIVATE
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
    GRAPH_SELECTION_ACTION_TEST_PLUGIN_PATH="$<TARGET_FILE:GraphSelectionActionTestPlugin>"
)
add_dependencies(IntelliGraphUnitTests GraphSelectionActionTestPlugin)

if (TARGET Qt${QT_VERSION_MAJOR}::QOffscreenIntegrationPlugin)
    add_custom_command(TARGET IntelliGraphUnitTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:IntelliGraphUnitTests>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QOffscreenIntegrationPlugin>"
                "$<TARGET_FILE_DIR:IntelliGraphUnitTests>/platforms/"
    )
endif()

include(GoogleTest)
gtest_discover_tests(IntelliGraphUnitTests TEST_PREFIX "IntelliGraph." DISCOVERY_MODE PRE_TEST)
