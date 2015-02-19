VERSION = $$QTAV_VERSION
*maemo*: DEFINES += Q_OS_MAEMO
# Add more folders to ship with the application, here
folder_01.source = qml/QMLPlayer
folder_01.target = qml
#will copy to target path
#DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

RESOURCES += \
    qmlplayer.qrc
TRANSLATIONS = i18n/QMLPlayer_zh_CN.ts

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp
lupdate_only{
SOURCES = qml/QMLPlayer/*.qml
}
# Installation path
target.path = $$[QT_INSTALL_BINS]


desktopfile.files = $$PWD/../../qtc_packaging/debian_generic/QMLPlayer.desktop
desktopfile.path = /usr/share/applications

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()

#!*msvc*: QMAKE_LFLAGS += -u __link_hack
isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..
include($${PROJECTROOT}/examples/common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
genRC($$TARGET)

DISTFILES += \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
