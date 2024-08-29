cmake_minimum_required (VERSION 3.15.0)

option(BUILD_LIB    "Build Lib"    ON)
option(BUILD_MODULE "Build Module" ON)

if (BUILD_LIB)
    add_subdirectory(lib)
endif ()
if (BUILD_MODULE)
    add_subdirectory(module)
endif ()
