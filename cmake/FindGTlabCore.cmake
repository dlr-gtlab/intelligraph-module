# - Find GTlabCore
# Find the native GTlabCore headers and libraries.
#
#  GTlabCore_FOUND          - True if GTlabCore found.
#
# Creates the CMake target GTlabCore::Core  GTlabCore::Gui  GTlabCore::DataProcessor for the GTlabCore library

include(CMakeFindDependencyMacro)
find_dependency(Qt5 COMPONENTS Core Gui)

# Look for the header file.
find_path(GTlab_INCLUDE_DIR NAMES gt_globals.h PATH_SUFFIXES core) 

# Look for the library (sorted from most current/relevant entry to least).
find_library(GTlab_Core_LIBRARY NAMES
    GTlabCore
    PATH_SUFFIXES core
)

find_library(GTlab_Core_LIBRARY_DEBUG NAMES
    GTlabCore-d
    PATH_SUFFIXES core
)

find_library(GTlab_DataProcessor_LIBRARY NAMES
    GTlabDataProcessor
    PATH_SUFFIXES core
)

find_library(GTlab_DataProcessor_LIBRARY_DEBUG NAMES
    GTlabDataProcessor-d
    PATH_SUFFIXES core
)


find_library(GTlab_Gui_LIBRARY NAMES
    GTlabGui
    PATH_SUFFIXES core
)

find_library(GTlab_Gui_LIBRARY_DEBUG NAMES
    GTlabGui-d
    PATH_SUFFIXES core
)


# handle the QUIETLY and REQUIRED arguments and set GTlabCore_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTlabCore
                                  REQUIRED_VARS GTlab_Core_LIBRARY GTlab_DataProcessor_LIBRARY  GTlab_Gui_LIBRARY GTlab_INCLUDE_DIR)

if(GTlabCore_FOUND)


  add_library(GTlab::DataProcessor UNKNOWN IMPORTED)
  set_target_properties(GTlab::DataProcessor PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${GTlab_INCLUDE_DIR}
      IMPORTED_LOCATION ${GTlab_DataProcessor_LIBRARY}
      IMPORTED_IMPLIB ${GTlab_DataProcessor_LIBRARY}
      IMPORTED_LOCATION_DEBUG ${GTlab_DataProcessor_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG ${GTlab_DataProcessor_LIBRARY_DEBUG}
  )

  add_library(GTlab::Core UNKNOWN IMPORTED)
  set_target_properties(GTlab::Core PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${GTlab_INCLUDE_DIR}
      IMPORTED_LOCATION ${GTlab_Core_LIBRARY}
      IMPORTED_IMPLIB ${GTlab_Core_LIBRARY}
      IMPORTED_LOCATION_DEBUG ${GTlab_Core_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG ${GTlab_Core_LIBRARY_DEBUG}
  )

  set_property(TARGET GTlab::Core APPEND PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES GTlab::DataProcessor GTlab::Core Qt5::Gui)

  add_library(GTlab::Gui UNKNOWN IMPORTED)
  set_target_properties(GTlab::Gui PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${GTlab_INCLUDE_DIR}
      IMPORTED_LOCATION ${GTlab_Gui_LIBRARY}
      IMPORTED_IMPLIB ${GTlab_Gui_LIBRARY}

      IMPORTED_LOCATION_DEBUG ${GTlab_Gui_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG ${GTlab_Gui_LIBRARY_DEBUG}

  )

  set_property(TARGET GTlab::Gui APPEND PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES GTlab::DataProcessor GTlab::Core Qt5::Gui)

endif()
