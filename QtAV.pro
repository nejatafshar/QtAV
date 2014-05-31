include(root.pri)

TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtav tools
libqtav.file = src/libQtAV.pro
greaterThan(QT_MAJOR_VERSION, 4) {
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += libqmlav
    libqmlav.file = qml/libQmlAV.pro
    libqmlav.depends += libqtav
    examples.depends += libqmlav
  }
}
!no-examples {
  SUBDIRS += examples
  examples.depends += libqtav
}
!no-tests {
  SUBDIRS += tests
  tests.depends += libqtav
}
OTHER_FILES += README.md TODO.txt Changelog
OTHER_FILES += templates/vo.h templates/vo.cpp templates/COPYRIGHT.h templates/mkclass.sh
OTHER_FILES += \
	templates/base.h templates/base.cpp templates/base_p.h \
	templates/derived.h templates/derived.cpp templates/derived_p.h \
	templates/final.h templates/final.cpp
#OTHER_FILES += config.test/mktest.sh


EssentialDepends = avutil avcodec avformat swscale
OptionalDepends = \
    swresample \
    avresample \
    gl
## sse2 sse4_1 may be defined in Qt5 qmodule.pri but is not included. Qt4 defines sse and sse2
!no-sse4_1:!sse4_1: OptionalDepends *= sse4_1
# no-xxx can set in $$PWD/user.conf
!no-openal: OptionalDepends *= openal
!no-portaudio: OptionalDepends *= portaudio
!no-direct2d: OptionalDepends *= direct2d
!no-gdiplus: OptionalDepends *= gdiplus
# why win32 is false?
!no-dxva: OptionalDepends *= dxva
unix {
    !no-xv: OptionalDepends *= xv
    !no-vaapi-x11: OptionalDepends *= vaapi-x11
    !no-cedarv: OptionalDepends *= libcedarv
}

runConfigTests()
!config_avresample:!config_swresample {
  error("libavresample or libswresample is required. Setup your environment correctly then delete $$BUILD_DIR/.qmake.conf and run qmake again")
}


PACKAGE_VERSION = 1.3.4
PACKAGE_NAME= QtAV

include(pack.pri)
#packageSet(1.3.4, QtAV)
