# - Find GTlabPreDesign
# Find the native GTlabPreDesign headers and libraries.
#
#  GTlabPreDesign_FOUND          - True if GTlabPreDesign found.
#
# Creates the CMake target GTlab::PreDesign

# Look for the header file.
find_path(GTlabPreDesign_INCLUDE_DIR NAMES gt_predesign.h PATH_SUFFIXES predesign) 

# Look for the library (sorted from most current/relevant entry to least).
find_library(GTlabPreDesign_LIBRARY NAMES
    GTlabPreDesign
    PATH_SUFFIXES predesign
)

find_library(GTlabPreDesign_LIBRARY_DEBUG NAMES
    GTlabPreDesign-d
    PATH_SUFFIXES predesign
)

# handle the QUIETLY and REQUIRED arguments and set GTlabCore_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTlabPreDesign
                                  REQUIRED_VARS GTlabPreDesign_LIBRARY  GTlabPreDesign_INCLUDE_DIR)

if(GTlabPreDesign_FOUND)


  add_library(GTlab::PreDesign UNKNOWN IMPORTED)
  set_target_properties(GTlab::PreDesign PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${GTlabPreDesign_INCLUDE_DIR}
      IMPORTED_LOCATION ${GTlabPreDesign_LIBRARY}
      IMPORTED_IMPLIB ${GTlabPreDesign_LIBRARY}
      IMPORTED_LOCATION_DEBUG ${GTlabPreDesign_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG ${GTlabPreDesign_LIBRARY_DEBUG}
  )

  set_property(TARGET GTlab::PreDesign APPEND PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES GTlab::Core)
  set_property(TARGET GTlab::PreDesign APPEND PROPERTY INTERFACE_COMPILE_OPTIONS -DGT_CADKERNEL)



endif()
