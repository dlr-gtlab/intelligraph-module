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

# Try to find DevTools (optionally)
# set via GTlabDevtools_ROOT=<path/to/devtools>
find_package(GTlabDevtools QUIET) # optional

if (NOT TARGET GTlab::Core)
    find_package(GTlab REQUIRED)
endif()

include(${GTlab_DIR}/GTlab.cmake)
gtlab_standard_setup()

set(Qt5_DIR ${QT_DIR})
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Concurrent Gui)

if (NOT TARGET GTlab::Logging)
    find_package(GTlabLogging REQUIRED)
endif()


add_subdirectory(src)
if (BUILD_UNITTESTS)
    add_subdirectory(tests/unittests)
endif ()
