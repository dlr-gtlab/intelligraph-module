set(CMAKE_DEBUG_POSTFIX -d)

set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(INTELLIGRAPH_VERSION 0.1.0)

add_library(GTlabIntelliGraph SHARED
    gt_intelligraphmodule.h
    gt_igglobals.h
    gt_igvolatileptr.h
    gt_intelligraph_exports.h
    gt_intelligraphnodefactory.h
    data/gt_igpackage.h
    data/gt_intelligraph.h
    data/gt_intelligraphconnection.h
    data/gt_intelligraphnode.h
    data/gt_intelligraphnodegroup.h
    data/gt_intelligraphcategory.h
    #data/nodes/gt_igabstractqwtnode.h
    #data/nodes/gt_igabstractshapenode.h
    #data/nodes/gt_igcombineshapesnode.h
    data/nodes/gt_igfinddirectchildnode.h
    data/nodes/gt_iggroupinputprovider.h
    data/nodes/gt_iggroupoutputprovider.h
    data/nodes/gt_igobjectmementonode.h
    data/nodes/gt_igobjectsourcenode.h
    #data/nodes/gt_igqwtbarchartnode.h
    #data/nodes/gt_igshapecolornode.h
    #data/nodes/gt_igshapegeneratornode.h
    #data/nodes/gt_igshapesettingsnode.h
    #data/nodes/gt_igshapevisualizationnode.h
    data/nodes/gt_igstringlistinputnode.h
    #data/nodes/gt_igwireframenode.h
    data/properties/gt_igobjectlinkproperty.h
    data/properties/gt_igcolorporperty.h
    gui/items/gt_intelligrapheditor.h
    #gui/nds_3dplot.h
    #gui/nds_barchartwidget.h
    gui/models/gt_intelligraphobjectmodel.h
    #gui/models/nds_examplemodel.h
    gui/models/data/gt_igobjectdata.h
    #gui/models/data/gt_igshapedata.h
    #gui/models/data/gt_igshapesettingsdata.h
    gui/models/data/gt_igstringlistdata.h
    gui/property_items/gt_igobjectlinkpropertyitem.h
    gui/ui/gt_intelligraphobjectui.h
    gui/ui/gt_igprojectui.h

    gt_intelligraphmodule.cpp
    gt_intelligraphnodefactory.cpp
    data/gt_igpackage.cpp
    data/gt_intelligraph.cpp
    data/gt_intelligraphconnection.cpp
    data/gt_intelligraphnode.cpp
    data/gt_intelligraphnodegroup.cpp
    data/gt_intelligraphcategory.cpp
    #data/nodes/gt_igabstractqwtnode.cpp
    #data/nodes/gt_igabstractshapenode.cpp
    #data/nodes/gt_igcombineshapesnode.cpp
    data/nodes/gt_iggroupinputprovider.cpp
    data/nodes/gt_igfinddirectchildnode.cpp
    data/nodes/gt_iggroupoutputprovider.cpp
    data/nodes/gt_igobjectmementonode.cpp
    data/nodes/gt_igobjectsourcenode.cpp
    #data/nodes/gt_igqwtbarchartnode.cpp
    #data/nodes/gt_igshapecolornode.cpp
    #data/nodes/gt_igshapegeneratornode.cpp
    #data/nodes/gt_igshapesettingsnode.cpp
    #data/nodes/gt_igshapevisualizationnode.cpp
    data/nodes/gt_igstringlistinputnode.cpp
    #data/nodes/gt_igwireframenode.cpp
    data/properties/gt_igobjectlinkproperty.cpp
    data/properties/gt_igcolorporperty.cpp
    gui/items/gt_intelligrapheditor.cpp
    #gui/nds_3dplot.cpp
    #gui/nds_barchartwidget.cpp
    gui/models/gt_intelligraphobjectmodel.cpp
    #gui/models/nds_examplemodel.cpp
    gui/property_items/gt_igobjectlinkpropertyitem.cpp
    gui/ui/gt_intelligraphobjectui.cpp
    gui/ui/gt_igprojectui.cpp
)

target_link_libraries(GTlabIntelliGraph
  PRIVATE
  Qt5::Xml
  GTlab::Logging
  GTlab::Gui
  QtNodes::QtNodes
)
set_property(TARGET GTlabIntelliGraph PROPERTY AUTOMOC ON)

if (${BUILD_SHARED_LIBS})
    target_compile_definitions(GTlabIntelliGraph PRIVATE GT_INTELLIGRAPH_DLL)
else()
    target_compile_definitions(GTlabIntelliGraph PUBLIC  GT_INTELLIGRAPH_STATIC)
endif()

target_compile_definitions(GTlabIntelliGraph PRIVATE
    GT_MODULE_ID="IntelliGraph"
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
)

target_include_directories(GTlabIntelliGraph PRIVATE
    .
    data
    data/properties
    gui
    gui/items
    gui/property_items
    gui/ui)

install (TARGETS GTlabIntelliGraph
    EXPORT GTlabIntelliGraph-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/intelligraph
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/intelligraph
)

if (IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/build)
    message(STATUS "Deploying into GTlab build dir")

    if (UNIX)
        install (TARGETS GTlabIntelliGraph
            LIBRARY DESTINATION build/modules
        )

    else(UNIX)
        install (TARGETS GTlabIntelliGraph
            RUNTIME DESTINATION build/modules
        )
    endif(UNIX)
else ()
    if (UNIX)
        install (TARGETS GTlabIntelliGraph
            LIBRARY DESTINATION
            $<IF:$<CONFIG:Debug>,binDebug/modules,bin/modules>
        )

    else(UNIX)
        install (TARGETS GTlabIntelliGraph
            RUNTIME DESTINATION
            $<IF:$<CONFIG:Debug>,binDebug/modules,bin/modules>
        )
    endif(UNIX)
endif ()


file(GLOB_RECURSE HEADERS
    "*.h"
)

install (FILES ${HEADERS} DESTINATION include/intelligraph)

# cmake configuration export
set(CMAKE_INSTALL_INCLUDE_DIR "include")
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/GTlabIntelliGraph")
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabIntelliGraphConfigVersion.cmake"
  VERSION ${INTELLIGRAPH_VERSION}
  COMPATIBILITY AnyNewerVersion
)

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/GTlabIntelliGraphConfig.cmake"
  INPUT  "${PROJECT_SOURCE_DIR}/cmake/GTlabIntelliGraphConfig.in"
)

install(EXPORT  GTlabIntelliGraph-targets
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install (FILES
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabIntelliGraphConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabIntelliGraphConfigVersion.cmake"
  DESTINATION ${CONFIG_INSTALL_DIR})
