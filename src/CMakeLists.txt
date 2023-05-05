set(CMAKE_DEBUG_POSTFIX -d)

set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX};${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/libDebug")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(NODES_VERSION 0.0.1)

add_library(GTlabNodesModule SHARED
    nds_nodesmodule.h
    gt_igglobals.h
    gt_igvolatileptr.h
    gt_intelligraphnodefactory.h
    data/nds_package.h
    data/gt_intelligraph.h
    data/gt_intelligraphconnection.h
    data/gt_intelligraphnode.h
    data/gt_intelligraphcategory.h
    data/nodes/gt_igabstractqwtnode.h
    data/nodes/gt_igabstractshapenode.h
    data/nodes/gt_igcombineshapesnode.h
    data/nodes/gt_igobjectmementonode.h
    data/nodes/gt_igobjectsourcenode.h
    data/nodes/gt_igqwtbarchartnode.h
    data/nodes/gt_igshapecolornode.h
    data/nodes/gt_igshapegeneratornode.h
    data/nodes/gt_igshapesettingsnode.h
    data/nodes/gt_igshapevisualizationnode.h
    data/nodes/gt_igstringlistinputnode.h
    data/nodes/gt_igwireframenode.h
    data/properties/gt_igobjectlinkproperty.h
    data/properties/gt_igcolorporperty.h
    gui/items/gt_intelligrapheditor.h
    gui/nds_3dplot.h
    gui/nds_barchartwidget.h
    gui/models/gt_intelligraphobjectmodel.h
    gui/models/nds_examplemodel.h
    gui/models/data/gt_igobjectdata.h
    gui/models/data/gt_igshapedata.h
    gui/models/data/gt_igshapesettingsdata.h
    gui/models/data/gt_igstringlistdata.h
    gui/ui/gt_intelligraphobjectui.h
    gui/ui/nds_projectui.h

    nds_nodesmodule.cpp
    gt_intelligraphnodefactory.cpp
    data/nds_package.cpp
    data/gt_intelligraph.cpp
    data/gt_intelligraphconnection.cpp
    data/gt_intelligraphnode.cpp
    data/gt_intelligraphcategory.cpp
    data/nodes/gt_igabstractqwtnode.cpp
    data/nodes/gt_igabstractshapenode.cpp
    data/nodes/gt_igcombineshapesnode.cpp
    data/nodes/gt_igobjectmementonode.cpp
    data/nodes/gt_igobjectsourcenode.cpp
    data/nodes/gt_igqwtbarchartnode.cpp
    data/nodes/gt_igshapecolornode.cpp
    data/nodes/gt_igshapegeneratornode.cpp
    data/nodes/gt_igshapesettingsnode.cpp
    data/nodes/gt_igshapevisualizationnode.cpp
    data/nodes/gt_igstringlistinputnode.cpp
    data/nodes/gt_igwireframenode.cpp
    data/properties/gt_igobjectlinkproperty.cpp
    data/properties/gt_igcolorporperty.cpp
    gui/items/gt_intelligrapheditor.cpp
    gui/nds_3dplot.cpp
    gui/nds_barchartwidget.cpp
    gui/models/gt_intelligraphobjectmodel.cpp
    gui/models/nds_examplemodel.cpp
    gui/ui/gt_intelligraphobjectui.cpp
    gui/ui/nds_projectui.cpp
)

target_link_libraries(GTlabNodesModule
  PRIVATE
    Qt5::Xml
    GTlab::Logging
    GTlab::Gui
    GTlab::CadKernel
    GTlab::PreDesign
    QtNodes::QtNodes
    qwt::qwt
)
set_property(TARGET GTlabNodesModule PROPERTY AUTOMOC ON)

target_compile_definitions(GTlabNodesModule PRIVATE
    GT_MODULE_ID="Nodes Module"
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
)

target_include_directories(GTlabNodesModule PRIVATE . data data/properties gui gui/items gui/ui)

install (TARGETS GTlabNodesModule
    EXPORT GTlabNodesModule-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/nodes
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/nodes
)

if (IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/src)
  message(STATUS "Deploying into GTlab build dir")

  install (TARGETS GTlabNodesModule
      RUNTIME DESTINATION "build/modules"
  )

else ()
    install (TARGETS GTlabNodesModule
        CONFIGURATIONS RELEASE
        RUNTIME DESTINATION bin/modules
    )

    install (TARGETS GTlabNodesModule
        CONFIGURATIONS DEBUG
        RUNTIME DESTINATION binDebug/modules
    )
endif ()


file(GLOB_RECURSE HEADERS
    "*.h"
)

install (FILES ${HEADERS} DESTINATION include/nodes)

# cmake configuration export
set(CMAKE_INSTALL_INCLUDE_DIR "include")
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/GTlabNodesModule")
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabNodesModuleConfigVersion.cmake"
  VERSION ${NODES_VERSION}
  COMPATIBILITY AnyNewerVersion
)

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/GTlabNodesModuleConfig.cmake"
  INPUT  "${PROJECT_SOURCE_DIR}/cmake/GTlabNodesModuleConfig.in"
)

install(EXPORT  GTlabNodesModule-targets
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install (FILES
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabNodesModuleConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/GTlabNodesModuleConfigVersion.cmake"
  DESTINATION ${CONFIG_INSTALL_DIR})
