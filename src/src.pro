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
    gt_igglobals.h \
    data/gt_intelligraph.h \
    data/gt_intelligraphconnection.h \
    data/gt_intelligraphnode.h \
    data/nodes/gt_igobjectsourcenode.h \
    gui/items/gt_intelligrapheditor.h \
    gui/items/nds_3dplot.h \
    gui/items/nds_barchartwidget.h \
    gui/items/models/nds_abstractqwtmodel.h \
    gui/items/models/nds_abstractshapemodel.h \
    gui/items/models/nds_combineshapesmodel.h \
    gui/items/models/nds_examplemodel.h \
    gui/items/models/nds_objectloadermodel.h \
    gui/items/models/nds_objectmementomodel.h \
    gui/items/models/nds_qwtbarchartmodel.h \
    gui/items/models/nds_shapegenmodel.h \
    gui/items/models/nds_shapesettingsmodel.h \
    gui/items/models/nds_shapevisualizationmodel.h \
    gui/items/models/nds_simplemodel.h \
    gui/items/models/nds_wireframemodel.h \
    gui/items/models/ndsshapecolormodel.h \
    gui/items/models/data/nds_shapedata.h \
    gui/items/models/data/nds_shapesettingsdata.h \
    gui/items/models/data/nds_objectdata.h \
    nds_nodesmodule.h \
    data/nds_package.h \
    gui/ui/nds_projectui.h 

SOURCES += \
    data/gt_intelligraph.cpp \
    data/gt_intelligraphconnection.cpp \
    data/gt_intelligraphnode.cpp \
    data/nodes/gt_igobjectsourcenode.cpp \
    gui/items/gt_intelligrapheditor.cpp \
    gui/items/nds_3dplot.cpp \
    gui/items/nds_barchartwidget.cpp \
    gui/items/models/nds_abstractqwtmodel.cpp \
    gui/items/models/nds_abstractshapemodel.cpp \
    gui/items/models/nds_combineshapesmodel.cpp \
    gui/items/models/nds_examplemodel.cpp \
    gui/items/models/nds_objectloadermodel.cpp \
    gui/items/models/nds_objectmementomodel.cpp \
    gui/items/models/nds_qwtbarchartmodel.cpp \
    gui/items/models/nds_shapegenmodel.cpp \
    gui/items/models/nds_shapesettingsmodel.cpp \
    gui/items/models/nds_shapevisualizationmodel.cpp \
    gui/items/models/nds_simplemodel.cpp \
    gui/items/models/nds_wireframemodel.cpp \
    gui/items/models/ndsshapecolormodel.cpp \
    nds_nodesmodule.cpp \
    data/nds_package.cpp \
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
