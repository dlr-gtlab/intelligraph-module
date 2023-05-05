# - Find GTlabCore
# Find the native GTlabLogging headers and libraries.
#
#  GTlabLogging_FOUND          - True if GTlabLogging found.
#
# Creates the CMake target for GTlabLogging library

# Look for the header file.
find_path(GTlab_Logging_INCLUDE_DIR NAMES gt_logging.h PATH_SUFFIXES logging) 

# Look for the library (sorted from most current/relevant entry to least).
find_library(GTlab_Logging_LIBRARY NAMES
    GTlabLogging
    PATH_SUFFIXES logging
)

find_library(GTlab_Logging_LIBRARY_DEBUG NAMES
    GTlabLogging-d
    PATH_SUFFIXES logging
)

# handle the QUIETLY and REQUIRED arguments and set GTlabCore_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTlabLogging
                                  REQUIRED_VARS GTlab_Logging_LIBRARY GTlab_Logging_INCLUDE_DIR)

if(GTlabLogging_FOUND)


  add_library(GTlab::Logging SHARED IMPORTED)
  set_target_properties(GTlab::Logging PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${GTlab_Logging_INCLUDE_DIR}
      IMPORTED_LOCATION ${GTlab_Logging_LIBRARY}
      IMPORTED_IMPLIB ${GTlab_Logging_LIBRARY}
      IMPORTED_LOCATION_DEBUG ${GTlab_Logging_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG ${GTlab_Logging_LIBRARY_DEBUG}
  )

endif()
