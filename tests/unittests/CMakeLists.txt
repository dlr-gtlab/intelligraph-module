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


add_executable(IntelliGraphUnitTests
    main.cpp
    strongtypes.cpp
    graphbuilder.cpp
)

target_link_libraries(IntelliGraphUnitTests PRIVATE GTlabIntelliGraph gtest)

add_test(NAME Unittests COMMAND IntelliGraphUnitTests)
