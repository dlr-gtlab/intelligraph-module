# make sure, static libraries can be linked into shared modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# force gtnodes building as a static library
set(BUILD_SHARED_LIBS OFF)

# make sure, that qtnodes does not overwrite
# BUILD_SHARED_LIBS variable
cmake_policy(SET CMP0077 NEW)
add_subdirectory(gtnodes EXCLUDE_FROM_ALL)
