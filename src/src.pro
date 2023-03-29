#             ______________      __
#            / ____/_  __/ /___  / /_
#           / / __  / / / / __ `/ __ \
#          / /_/ / / / / / /_/ / /_/ /
#          \____/ /_/ /_/\__,_/_.___/

######################################################################
#### DO NOT CHANGE!
######################################################################

include($${PWD}/../settings.pri)

CONFIG(debug, debug|release) {
    TARGET = NdsNodesModule-d
} else {
    TARGET = NdsNodesModule
}

QT += core gui widgets xml

# global define for module id
GT_MODULE_ID = Nodes Module
DEFINES += GT_MODULE_ID='"\\\"$${GT_MODULE_ID}\\\""'

DEFINES += NODE_EDITOR_SHARED
DEFINES += GT_CADKERNEL
DEFINES += QWT_DLL

TEMPLATE = lib
CONFIG += plugin
CONFIG += silent
CONFIG += c++14

CONFIG(debug, debug|release) {
    MOC_BUILD_DEST = ../build/debug
} else {
    MOC_BUILD_DEST = ../build/release
}

OBJECTS_DIR = $${MOC_BUILD_DEST}/obj
MOC_DIR = $${MOC_BUILD_DEST}/moc
RCC_DIR = $${MOC_BUILD_DEST}/rcc
UI_DIR  = $${MOC_BUILD_DEST}/ui
DESTDIR = ../$${LIB_BUILD_DEST}

INCLUDEPATH += . \
    data \
    gui/items \
    gui/ui 

HEADERS += \
    gui/items/nds_3dplot.h \
    gui/items/nds_abstractqwtmodel.h \
    gui/items/nds_abstractshapemodel.h \
    gui/items/nds_barchartwidget.h \
    gui/items/nds_combineshapesmodel.h \
#    gui/items/nds_helloqmlmodel.h \
    gui/items/nds_examplemodel.h \
    gui/items/nds_objectdata.h \
    gui/items/nds_objectloadermodel.h \
    gui/items/nds_objectmementomodel.h \
#    gui/items/nds_qmlbarchartmodel.h \
#    gui/items/nds_qmlpiechartmodel.h \
    gui/items/nds_qwtbarchartmodel.h \
    gui/items/nds_shapedata.h \
    gui/items/nds_shapegenmodel.h \
    gui/items/nds_shapesettingsdata.h \
    gui/items/nds_shapesettingsmodel.h \
    gui/items/nds_shapevisualizationmodel.h \
    gui/items/nds_simplemodel.h \
    gui/items/nds_wireframemodel.h \
#    gui/items/ndsqmllinechartmodel.h \
    gui/items/ndsshapecolormodel.h \
    nds_nodesmodule.h \
    data/nds_package.h \
    gui/items/nds_nodeeditor.h \
    gui/ui/nds_projectui.h 

SOURCES += \
    gui/items/nds_3dplot.cpp \
    gui/items/nds_abstractqwtmodel.cpp \
    gui/items/nds_abstractshapemodel.cpp \
    gui/items/nds_barchartwidget.cpp \
    gui/items/nds_combineshapesmodel.cpp \
#    gui/items/nds_helloqmlmodel.cpp \
    gui/items/nds_examplemodel.cpp \
    gui/items/nds_objectloadermodel.cpp \
    gui/items/nds_objectmementomodel.cpp \
#    gui/items/nds_qmlbarchartmodel.cpp \
#    gui/items/nds_qmlpiechartmodel.cpp \
    gui/items/nds_qwtbarchartmodel.cpp \
    gui/items/nds_shapegenmodel.cpp \
    gui/items/nds_shapesettingsmodel.cpp \
    gui/items/nds_shapevisualizationmodel.cpp \
    gui/items/nds_simplemodel.cpp \
    gui/items/nds_wireframemodel.cpp \
#    gui/items/ndsqmllinechartmodel.cpp \
    gui/items/ndsshapecolormodel.cpp \
    nds_nodesmodule.cpp \
    data/nds_package.cpp \
    gui/items/nds_nodeeditor.cpp \
    gui/ui/nds_projectui.cpp 
	
RESOURCES += \
    qml/qml.qrc

message(Targeting Major Version: $${MAJOR_VERSION})

CONFIG(debug, debug|release){
    # GTLAB CORE
    LIBS += -lGTlabLogging-d
    LIBS += -lGTlabNumerics-d

    LIBS += -lGTlabDataProcessor-d -lGTlabCore-d -lGTlabGui-d
    # Other
    LIBS += -lGTlabBasicTools-d
    LIBS += -lGTlabPerformance-d
    LIBS += -lGTlabPreDesign-d

    LIBS += -lGTlabCadKernel-d

    win32 {
        LIBS += -lqwtd
    }
    unix {
        LIBS += -lqwt
    }
} else {
    # GTLAB CORE
    LIBS += -lGTlabLogging
    LIBS += -lGTlabNumerics

    LIBS += -lGTlabDataProcessor -lGTlabCore -lGTlabGui
    # Other
    LIBS += -lGTlabBasicTools
    LIBS += -lGTlabPerformance
    LIBS += -lGTlabPreDesign

    LIBS += -lGTlabCadKernel

    # THIRD PARTY
    LIBS += -lqwt
}

LIBS += -lQtNodes

# HDF5
#CONFIG(debug, debug|release) {
#    # C/C+ API
#    win32:LIBS += -lhdf5_D -lhdf5_cpp_D
#    unix:LIBS += -lhdf5 -lhdf5_cpp
#    # Wrapper
#    LIBS += -lGenH5-d
#} else {
#    # C/C+ API
#    LIBS += -lhdf5 -lhdf5_cpp
#    # Wrapper
#    LIBS += -lGenH5
#}

unix:{
    # suppress the default RPATH if you wish
    QMAKE_LFLAGS_RPATH=
    # add your own with quoting gyrations to make sure $ORIGIN gets to the command line unexpanded
    QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN:\$$ORIGIN/..\''
}

######################################################################

copyHeaders($$HEADERS)
copyToEnvironmentPathModules()

######################################################################
