cmake_minimum_required (VERSION 3.15.0)

project(GTlabIntelliGraph)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

set(Qt5_DIR ${QT_DIR})

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Xml)

find_package(GTlabLogging REQUIRED)
find_package(GTlabCore REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(src)
