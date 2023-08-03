cmake_minimum_required (VERSION 3.15.0)

project(GTlabIntelliGraph)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

set(Qt5_DIR ${QT_DIR})

include(GTlab)
gtlab_standard_setup()
enable_gtlab_devtools()

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Concurrent Gui Xml)

find_package(GTlabLogging REQUIRED)
find_package(GTlab REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(src)
