# GTlab IntelliGraph
#
#  SPDX-License-Identifier: BSD-3-Clause
#  SPDX-FileCopyrightText: 2024 German Aerospace Center

set(CMAKE_DEBUG_POSTFIX -d)

set(INTELLIGRAPH_VERSION 0.14.0)

option(
    INTELLIGRAPH_DEBUG_NODE_GRAPHICS
    "Draws debugging information into the scene to show bounds of node objects" OFF
)
option(
    INTELLIGRAPH_DEBUG_CONNECTION_GRAPHICS
    "Draws debugging information into the scene to show bounds of connection objects" OFF
)
option(
    INTELLIGRAPH_DEBUG_NODE_EXECUTION
    "Logs additional debugging information when execution graphs" OFF
)
option(
    INTELLIGRAPH_DEBUG_NODE_PROPERTIES
    "Shows node properties in property dock widget" OFF
)
option(
    INTELLIGRAPH_ENABLE_EXPERIMENTAL_NODES
    "Enables nodes that are considered experimental and should not be used in production" OFF
)

set(PUBLIC_HEADERS
    intelli/core.h
    intelli/dynamicnode.h
    intelli/exports.h
    intelli/future.h
    intelli/globals.h
    intelli/graph.h
    intelli/graphbuilder.h
    intelli/graphconnectionmodel.h
    intelli/graphdatamodel.h
    intelli/graphexecmodel.h
    intelli/graphuservariables.h
    intelli/graphutilities.h
    intelli/memory.h
    intelli/node.h
    intelli/nodedata.h
    intelli/nodedatafactory.h
    intelli/nodedatainterface.h
    intelli/nodefactory.h
    intelli/span.h
    intelli/utilities.h
    intelli/view.h
    intelli/data/bool.h
    intelli/data/bytearray.h
    intelli/data/bytearraydata.h
    intelli/data/double.h
    intelli/data/file.h
    intelli/data/int.h
    intelli/data/integer.h
    intelli/data/invalid.h
    intelli/data/object.h
    intelli/data/string.h
    intelli/data/stringlist.h
    intelli/gui/connectiongeometry.h
    intelli/gui/connectionpainter.h
    intelli/gui/icons.h
    intelli/gui/graphscenedata.h
    intelli/gui/nodegeometry.h
    intelli/gui/nodepainter.h
    intelli/gui/nodeui.h
    intelli/gui/nodeuidata.h
    intelli/gui/portuiaction.h
    intelli/gui/style.h
    intelli/gui/graphics/graphicsobject.h
    intelli/gui/graphics/interactableobject.h
    intelli/gui/graphics/nodeobject.h
    intelli/node/input/boolinput.h
    intelli/node/input/doubleinput.h
    intelli/node/input/intinput.h
    intelli/node/input/objectinput.h
    intelli/node/input/stringinput.h
    intelli/property/color.h
    intelli/property/metaenum.h
    intelli/property/stringselection.h
    intelli/property/uint.h
)

qt5_add_resources (RCC_SOURCES
    ./resources/icons.qrc
)

add_gtlab_module(GTlabIntelliGraph MODULE_ID "IntelliGraph"
    SOURCES
    ${PUBLIC_HEADERS}
    ${RCC_SOURCES}
    intelli/connection.h
    intelli/connectiongroup.h
    intelli/graphcategory.h
    intelli/module.h
    intelli/package.h
    intelli/calculators/graphexeccalculator.h
    intelli/exec/detachedexecutor.h
    intelli/exec/dummynodedatamodel.h
    intelli/node/abstractgroupprovider.h
    intelli/node/booldisplay.h
    intelli/node/dummy.h
    intelli/node/existingdirectorysource.h
    intelli/node/filereader.h
    intelli/node/filewriter.h
    intelli/node/finddirectchild.h
    intelli/node/genericcalculatorexec.h
    intelli/node/groupinputprovider.h
    intelli/node/groupoutputprovider.h
    intelli/node/logicoperation.h
    intelli/node/numberdisplay.h
    intelli/node/numbermath.h
    intelli/node/objectmemento.h
    intelli/node/objectsink.h
    intelli/node/projectinfo.h
    intelli/node/sleepy.h
    intelli/node/stringbuilder.h
    intelli/node/stringselection.h
    intelli/node/textdisplay.h
    intelli/node/input/fileinput.h
    intelli/gui/commentgroup.h
    intelli/gui/commentdata.h
    intelli/gui/grapheditor.h
    intelli/gui/graphscene.h
    intelli/gui/graphscenemanager.h
    intelli/gui/graphsceneselector.h
    intelli/gui/graphstatemanager.h
    intelli/gui/graphview.h
    intelli/gui/graphviewoverlay.h
    intelli/gui/guidata.h
    intelli/gui/ui/commentui.h
    intelli/gui/ui/connectionui.h
    intelli/gui/ui/guidataui.h
    intelli/gui/ui/graphcategoryui.h
    intelli/gui/ui/logicnodeui.h
    intelli/gui/ui/packageui.h
    intelli/gui/graphics/commentobject.h
    intelli/gui/graphics/connectionobject.h
    intelli/gui/graphics/lineobject.h
    intelli/gui/graphics/nodeevalstateobject.h
    intelli/gui/graphics/popupitem.h
    intelli/gui/widgets/abstractnumberinputwidget.h
    intelli/gui/widgets/booldisplaywidget.h
    intelli/gui/widgets/doubleinputwidget.h
    intelli/gui/widgets/editablelabel.h
    intelli/gui/widgets/finddirectchildwidget.h
    intelli/gui/widgets/graphuservariablesdialog.h
    intelli/gui/widgets/intinputwidget.h
    intelli/gui/property_item/stringselection.h
    intelli/private/graph_impl.h
    intelli/private/graphexecmodel_impl.h
    intelli/private/node_impl.h
    intelli/private/utils.h
    intelli/private/gui_utils.h

    intelli/connection.cpp
    intelli/connectiongroup.cpp
    intelli/core.cpp
    intelli/dynamicnode.cpp
    intelli/future.cpp
    intelli/graph.cpp
    intelli/graphbuilder.cpp
    intelli/graphcategory.cpp
    intelli/graphexecmodel.cpp
    intelli/graphuservariables.cpp
    intelli/graphutilities.cpp
    intelli/module.cpp
    intelli/node.cpp
    intelli/nodedata.cpp
    intelli/nodedatafactory.cpp
    intelli/nodefactory.cpp
    intelli/package.cpp
    intelli/calculators/graphexeccalculator.cpp
    intelli/exec/detachedexecutor.cpp
    intelli/exec/dummynodedatamodel.cpp
    intelli/data/bool.cpp
    intelli/data/bytearray.cpp
    intelli/data/double.cpp
    intelli/data/int.cpp
    intelli/data/invalid.cpp
    intelli/data/file.cpp
    intelli/data/object.cpp
    intelli/data/string.cpp
    intelli/data/stringlist.cpp
    intelli/node/booldisplay.cpp
    intelli/node/dummy.cpp
    intelli/node/existingdirectorysource.cpp
    intelli/node/filereader.cpp
    intelli/node/filewriter.cpp
    intelli/node/finddirectchild.cpp
    intelli/node/genericcalculatorexec.cpp
    intelli/node/groupinputprovider.cpp
    intelli/node/groupoutputprovider.cpp
    intelli/node/logicoperation.cpp
    intelli/node/numberdisplay.cpp
    intelli/node/numbermath.cpp
    intelli/node/objectmemento.cpp
    intelli/node/objectsink.cpp
    intelli/node/projectinfo.cpp
    intelli/node/sleepy.cpp
    intelli/node/stringbuilder.cpp
    intelli/node/stringselection.cpp
    intelli/node/textdisplay.cpp
    intelli/node/input/boolinput.cpp
    intelli/node/input/doubleinput.cpp
    intelli/node/input/fileinput.cpp
    intelli/node/input/intinput.cpp
    intelli/node/input/objectinput.cpp
    intelli/node/input/stringinput.cpp
    intelli/gui/connectiongeometry.cpp
    intelli/gui/connectionpainter.cpp
    intelli/gui/commentgroup.cpp
    intelli/gui/commentdata.cpp
    intelli/gui/nodegeometry.cpp
    intelli/gui/nodepainter.cpp
    intelli/gui/nodeui.cpp
    intelli/gui/nodeuidata.cpp
    intelli/gui/grapheditor.cpp
    intelli/gui/graphscene.cpp
    intelli/gui/graphscenemanager.cpp
    intelli/gui/graphsceneselector.cpp
    intelli/gui/graphstatemanager.cpp
    intelli/gui/graphview.cpp
    intelli/gui/graphviewoverlay.cpp
    intelli/gui/guidata.cpp
    intelli/gui/icons.cpp
    intelli/gui/style.cpp
    intelli/gui/ui/commentui.cpp
    intelli/gui/ui/connectionui.cpp
    intelli/gui/ui/guidataui.cpp
    intelli/gui/ui/logicnodeui.cpp
    intelli/gui/ui/graphcategoryui.cpp
    intelli/gui/ui/packageui.cpp
    intelli/gui/graphics/commentobject.cpp
    intelli/gui/graphics/connectionobject.cpp
    intelli/gui/graphics/graphicsobject.cpp
    intelli/gui/graphics/interactableobject.cpp
    intelli/gui/graphics/lineobject.cpp
    intelli/gui/graphics/nodeevalstateobject.cpp
    intelli/gui/graphics/nodeobject.cpp
    intelli/gui/graphics/popupitem.cpp
    intelli/gui/widgets/abstractnumberinputwidget.cpp
    intelli/gui/widgets/booldisplaywidget.cpp
    intelli/gui/widgets/doubleinputwidget.cpp
    intelli/gui/widgets/editablelabel.cpp
    intelli/gui/widgets/finddirectchildwidget.cpp
    intelli/gui/widgets/graphuservariablesdialog.cpp
    intelli/gui/widgets/intinputwidget.cpp
    intelli/gui/property_item/stringselection.cpp
    intelli/property/color.cpp
    intelli/property/metaenum.cpp
    intelli/property/stringselection.cpp
    intelli/property/uint.cpp

    intelli/node/binarydisplay.h intelli/node/binarydisplay.cpp
)

target_link_libraries(GTlabIntelliGraph
    PUBLIC
    Qt5::Core
    GTlab::Logging
    GTlab::Gui
    PRIVATE
    Qt5::Concurrent
)
set_property(TARGET GTlabIntelliGraph PROPERTY AUTOMOC ON)

target_compile_definitions(GTlabIntelliGraph PRIVATE
    GT_LOG_USE_QT_BINDINGS # enable Qt operator<< for logging
)

if (INTELLIGRAPH_DEBUG_NODE_GRAPHICS)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_DEBUG_NODE_GRAPHICS
    )
endif()
if (INTELLIGRAPH_DEBUG_CONNECTION_GRAPHICS)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_DEBUG_CONNECTION_GRAPHICS
    )
endif()
if (INTELLIGRAPH_DEBUG_NODE_EXECUTION)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_DEBUG_NODE_EXEC
    )
endif()
if (INTELLIGRAPH_DEBUG_NODE_PROPERTIES)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_DEBUG_NODE_PROPERTIES
    )
endif()
if (INTELLIGRAPH_ENABLE_EXPERIMENTAL_NODES)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_EXPERIMENTAL_NODES
    )
endif()

target_include_directories(GTlabIntelliGraph PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    # we install a flat file hierachy, so we include only this folder
    PUBLIC $<INSTALL_INTERFACE:include>
)

install (TARGETS GTlabIntelliGraph
    EXPORT GTlabIntelliGraph-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# copy public headers but keep file hierarchy
foreach ( file ${PUBLIC_HEADERS} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION include/${dir} )
endforeach()

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
