set(CMAKE_DEBUG_POSTFIX -d)


set(INTELLIGRAPH_VERSION 0.1.0)

set(PUBLIC_HEADERS
    gt_igglobals.h
    gt_igvolatileptr.h
    gt_intelligraph_exports.h
    gt_intelligraphnodefactory.h
    gt_intelligraphdatafactory.h
    data/gt_intelligraphdynamicnode.h
    data/gt_intelligraphnode.h
    data/node_data/gt_igbooldata.h
    data/node_data/gt_igdoubledata.h
    data/node_data/gt_ignodedata.h
    data/node_data/gt_igobjectdata.h
    data/node_data/gt_igstringlistdata.h
    data/properties/gt_igcolorproperty.h
    data/properties/gt_igobjectlinkproperty.h
    data/properties/gt_igstringselectionproperty.h
    exec/gt_intelligraphexecutorfactory.h
    gui/gt_igicons.h
    gui/gt_igportuiaction.h
    gui/ui/gt_intelligraphnodeui.h
)

 qt5_add_resources (RCC_SOURCES
     ./resources/icons.qrc
 )

add_gtlab_module(GTlabIntelliGraph MODULE_ID "IntelliGraph"
    SOURCES
    ${PUBLIC_HEADERS}
    ${RCC_SOURCES}
    gt_intelligraphmodule.h
    gt_intelligraphmodeladapter.h
    gt_intelligraphjsonadapter.h
    data/gt_igpackage.h
    data/gt_intelligraph.h
    data/gt_intelligraphconnection.h
    data/gt_intelligraphconnectiongroup.h
    data/gt_intelligraphcategory.h
    data/nodes/gt_igabstractgroupprovider.h
    data/nodes/gt_igconditionalnode.h
    data/nodes/gt_igdoubletoboolnode.h
    data/nodes/gt_igfinddirectchildnode.h
    data/nodes/gt_iggroupinputprovider.h
    data/nodes/gt_iggroupoutputprovider.h
    data/nodes/gt_ignumberdisplaynode.h
    data/nodes/gt_ignumbersourcenode.h
    data/nodes/gt_igobjectmementonode.h
    data/nodes/gt_igobjectsourcenode.h
    data/nodes/gt_igsleepynode.h
    data/nodes/gt_igstringlistinputnode.h
    exec/gt_intelligraphexecutor.h
    exec/gt_intelligraphexecutorfactory.cpp
    exec/gt_intelligraphparallelexecutor.h
    exec/gt_intelligraphsequentialexecutor.h
    gui/gt_igicons.cpp
    gui/gt_intelligraphscene.h
    gui/gt_intelligraphview.h
    gui/items/gt_intelligrapheditor.h
    gui/models/gt_intelligraphobjectmodel.h
    gui/property_items/gt_igobjectlinkpropertyitem.h
    gui/property_items/gt_igstringselectionpropertyitem.h
    gui/ui/gt_intelligraphconnectionui.h
    gui/ui/gt_intelligraphpackageui.h
    private/intelligraphnode_impl.h
    private/utils.h

    gt_intelligraphmodule.cpp
    gt_intelligraphnodefactory.cpp
    gt_intelligraphjsonadapter.cpp
    gt_intelligraphdatafactory.cpp
    gt_intelligraphmodeladapter.cpp
    data/gt_igpackage.cpp
    data/gt_intelligraph.cpp
    data/gt_intelligraphconnection.cpp
    data/gt_intelligraphconnectiongroup.cpp
    data/gt_intelligraphdynamicnode.cpp
    data/gt_intelligraphnode.cpp
    data/gt_intelligraphcategory.cpp
    data/nodes/gt_igconditionalnode.cpp
    data/nodes/gt_igdoubletoboolnode.cpp
    data/nodes/gt_igfinddirectchildnode.cpp
    data/nodes/gt_iggroupinputprovider.cpp
    data/nodes/gt_iggroupoutputprovider.cpp
    data/nodes/gt_ignumberdisplaynode.cpp
    data/nodes/gt_ignumbersourcenode.cpp
    data/nodes/gt_igobjectmementonode.cpp
    data/nodes/gt_igobjectsourcenode.cpp
    data/nodes/gt_igsleepynode.cpp
    data/nodes/gt_igstringlistinputnode.cpp
    data/node_data/gt_igbooldata.cpp
    data/node_data/gt_igdoubledata.cpp
    data/node_data/gt_ignodedata.cpp
    data/node_data/gt_igobjectdata.cpp
    data/node_data/gt_igstringlistdata.cpp
    data/properties/gt_igcolorproperty.cpp
    data/properties/gt_igobjectlinkproperty.cpp
    data/properties/gt_igstringselectionproperty.cpp
    exec/gt_intelligraphexecutor.cpp
    exec/gt_intelligraphparallelexecutor.cpp
    exec/gt_intelligraphsequentialexecutor.cpp
    gui/gt_intelligraphscene.cpp
    gui/gt_intelligraphview.cpp
    gui/items/gt_intelligrapheditor.cpp
    gui/models/gt_intelligraphobjectmodel.cpp
    gui/property_items/gt_igobjectlinkpropertyitem.cpp
    gui/property_items/gt_igstringselectionpropertyitem.cpp
    gui/ui/gt_intelligraphconnectionui.cpp
    gui/ui/gt_intelligraphnodeui.cpp
    gui/ui/gt_intelligraphpackageui.cpp
)

target_link_libraries(GTlabIntelliGraph
  PUBLIC
  Qt5::Core
  GTlab::Logging
  GTlab::Gui
  PRIVATE
  Qt5::Concurrent
  QtNodes::QtNodes
)
set_property(TARGET GTlabIntelliGraph PROPERTY AUTOMOC ON)

target_compile_definitions(GTlabIntelliGraph PRIVATE
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
)

target_include_directories(GTlabIntelliGraph PUBLIC

     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data/nodes>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data/node_data>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data/properties>
     # TODO. what is really public, what is private???
    PRIVATE
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/exec>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/items>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/property_items>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/ui>

     # we install a flat file hierachy, so we include only this folder
     PUBLIC $<INSTALL_INTERFACE:include/intelligraph>
)

install (TARGETS GTlabIntelliGraph
    EXPORT GTlabIntelliGraph-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/intelligraph
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/intelligraph
)

install (FILES ${PUBLIC_HEADERS} DESTINATION include/intelligraph)

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
