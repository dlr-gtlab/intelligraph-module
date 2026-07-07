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

include(FetchContent)

set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TF_BUILD_CUDA OFF CACHE BOOL "" FORCE)
set(TF_BUILD_SYCL OFF CACHE BOOL "" FORCE)
set(TF_BUILD_PROFILER OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    taskflow
    GIT_REPOSITORY https://github.com/taskflow/taskflow.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(taskflow)

# Try to find DevTools (optionally)
# set via GTlabDevtools_ROOT=<path/to/devtools>
find_package(GTlabDevtools QUIET) # optional

if (NOT TARGET GTlab::Core)
    find_package(GTlab REQUIRED)
endif()

require_qt(REQUIRED COMPONENTS Widgets Core Concurrent Gui)

gtlab_standard_setup()

if (NOT TARGET GTlab::Logging)
    find_package(GTlabLogging REQUIRED)
endif()

add_subdirectory(src)
if (BUILD_UNITTESTS)
    add_subdirectory(tests/unittests)
endif ()
