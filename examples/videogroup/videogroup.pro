TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG -= app_bundle

TARGET = videogroup
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp videogroup.cpp
HEADERS += videogroup.h



