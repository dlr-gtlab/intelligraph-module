# make sure, static libraries can be linked into shared modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# force gtnodes building as a static library
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(gtnodes EXCLUDE_FROM_ALL)
