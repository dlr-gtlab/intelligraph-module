set(CMAKE_DEBUG_POSTFIX -d)

set(INTELLIGRAPH_VERSION 0.8.0)

option(
    _DEBUG_NODE_GRAPHICS
    "Draws debugging information into the scene to show bounds of node and connection objects" OFF
)

set(PUBLIC_HEADERS
    intelli/core.h
    intelli/dynamicnode.h
    intelli/exports.h
    intelli/globals.h
    intelli/graph.h
    intelli/graphbuilder.h
    intelli/graphexecmodel.h
    intelli/memory.h
    intelli/node.h
    intelli/nodedata.h
    intelli/nodedatafactory.h
    intelli/nodedatainterface.h
    intelli/nodeexecutor.h
    intelli/nodefactory.h
    intelli/data/bool.h
    intelli/data/bytearraydata.h
    intelli/data/double.h
    intelli/data/object.h
    intelli/data/int.h
    intelli/data/integer.h
    intelli/data/stringlist.h
    intelli/data/string.h
    intelli/gui/icons.h
    intelli/gui/nodegeometry.h
    intelli/gui/nodepainter.h
    intelli/gui/nodeui.h
    intelli/gui/portuiaction.h
    intelli/gui/style.h
    intelli/gui/graphics/nodeobject.h
    intelli/property/color.h
    intelli/property/objectlink.h
    intelli/property/stringselection.h
    intelli/property/uint.h

    intelli/node/propertyinput/abstractinputnode.h
    intelli/node/propertyinput/boolinputnode.h
    intelli/node/propertyinput/doubleinputnode.h
    intelli/node/propertyinput/intinputnode.h
    intelli/node/propertyinput/objectinputnode.h
    intelli/node/propertyinput/stringinputnode.h
)

qt5_add_resources (RCC_SOURCES
    ./resources/icons.qrc
)

add_gtlab_module(GTlabIntelliGraph MODULE_ID "IntelliGraph"
    SOURCES
    ${PUBLIC_HEADERS}
    ${RCC_SOURCES}
    intelli/package.h
    intelli/connection.h
    intelli/connectiongroup.h
    intelli/graphcategory.h
    intelli/module.h
    intelli/calculators/graphexeccalculator.h
    intelli/exec/detachedexecutor.h
    intelli/node/abstractgroupprovider.h
    intelli/node/finddirectchild.h
    intelli/node/groupinputprovider.h
    intelli/node/groupoutputprovider.h
    intelli/node/logicdisplay.h
    intelli/node/logicoperation.h
    intelli/node/logicsource.h
    intelli/node/numberdisplay.h
    intelli/node/numbermath.h
    intelli/node/numbersource.h
    intelli/node/objectmemento.h
    intelli/node/objectsource.h
    intelli/node/sleepy.h
    intelli/node/stringlistinput.h
    intelli/node/existingdirectorysource.h
    intelli/gui/connectionui.h
    intelli/gui/grapheditor.h
    intelli/gui/graphscene.h
    intelli/gui/graphview.h
    intelli/gui/packageui.h
    intelli/gui/graphics/connectionobject.h
    intelli/gui/graphics/nodeevalstateobject.h
    intelli/gui/property_item/abstractnumberinputwidget.h
    intelli/gui/property_item/doubleinputwidget.h
    intelli/gui/property_item/editabledoublelabel.h
    intelli/gui/property_item/editableintegerlabel.h
    intelli/gui/property_item/integerinputwidget.h
    intelli/gui/property_item/logic.h
    intelli/gui/property_item/objectlink.h
    intelli/gui/property_item/stringselection.h
    intelli/gui/ui/logicnodeui.h
    intelli/private/node_impl.h
    intelli/private/utils.h

    intelli/connection.cpp
    intelli/connectiongroup.cpp
    intelli/core.cpp
    intelli/dynamicnode.cpp
    intelli/graph.cpp
    intelli/graphbuilder.cpp
    intelli/graphcategory.cpp
    intelli/graphexecmodel.cpp
    intelli/module.cpp
    intelli/node.cpp
    intelli/nodedata.cpp
    intelli/nodedatafactory.cpp
    intelli/nodedatainterface.cpp
    intelli/nodeexecutor.cpp
    intelli/nodefactory.cpp
    intelli/package.cpp
    intelli/calculators/graphexeccalculator.cpp
    intelli/exec/detachedexecutor.cpp
    intelli/data/bool.cpp
    intelli/data/double.cpp
    intelli/data/int.cpp
    intelli/data/object.cpp
    intelli/data/stringlist.cpp
    intelli/data/string.cpp
    intelli/node/finddirectchild.cpp
    intelli/node/groupinputprovider.cpp
    intelli/node/groupoutputprovider.cpp
    intelli/node/logicdisplay.cpp
    intelli/node/logicoperation.cpp
    intelli/node/logicsource.cpp
    intelli/node/numberdisplay.cpp
    intelli/node/numbermath.cpp
    intelli/node/numbersource.cpp
    intelli/node/objectmemento.cpp
    intelli/node/objectsource.cpp
    intelli/node/sleepy.cpp
    intelli/node/stringlistinput.cpp
    intelli/node/existingdirectorysource.cpp
    intelli/node/propertyinput/boolinputnode.cpp
    intelli/node/propertyinput/doubleinputnode.cpp
    intelli/node/propertyinput/intinputnode.cpp
    intelli/node/propertyinput/objectinputnode.cpp
    intelli/node/propertyinput/stringinputnode.cpp
    intelli/gui/connectionui.cpp
    intelli/gui/nodegeometry.cpp
    intelli/gui/nodepainter.cpp
    intelli/gui/nodeui.cpp
    intelli/gui/grapheditor.cpp
    intelli/gui/graphscene.cpp
    intelli/gui/graphview.cpp
    intelli/gui/icons.cpp
    intelli/gui/packageui.cpp
    intelli/gui/style.cpp
    intelli/gui/graphics/connectionobject.cpp
    intelli/gui/graphics/nodeobject.cpp
    intelli/gui/graphics/nodeevalstateobject.cpp
    intelli/gui/property_item/abstractnumberinputwidget.cpp
    intelli/gui/property_item/doubleinputwidget.cpp
    intelli/gui/property_item/editabledoublelabel.cpp
    intelli/gui/property_item/editableintegerlabel.cpp
    intelli/gui/property_item/integerinputwidget.cpp
    intelli/gui/property_item/logic.cpp
    intelli/gui/property_item/objectlink.cpp
    intelli/gui/property_item/stringselection.cpp
    intelli/gui/ui/logicnodeui.cpp
    intelli/property/color.cpp
    intelli/property/objectlink.cpp
    intelli/property/stringselection.cpp
    intelli/property/uint.cpp
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

if (_DEBUG_NODE_GRAPHICS)
    target_compile_definitions(GTlabIntelliGraph PRIVATE
        GT_INTELLI_DEBUG_GRAPHICS
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
