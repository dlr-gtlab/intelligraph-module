# GTlab IntelliGraph
#
#  SPDX-License-Identifier: BSD-3-Clause
#  SPDX-FileCopyrightText: 2024 German Aerospace Center

cmake_minimum_required (VERSION 3.15.0)

project(GTlabIntelliGraph)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_AUTOMOC ON)

option(BUILD_UNITTESTS "Build Unittests" OFF)

include(GNUInstallDirs)

set(Qt5_DIR ${QT_DIR})

include(GTlab)
gtlab_standard_setup()
enable_gtlab_devtools()

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Concurrent Gui)

if (NOT TARGET GTlab::Logging)
    find_package(GTlabLogging REQUIRED)
endif()

if (NOT TARGET GTlab::Core)
    find_package(GTlab REQUIRED)
endif()

add_subdirectory(src)
if (BUILD_UNITTESTS)
    add_subdirectory(tests/unittests)
endif ()
