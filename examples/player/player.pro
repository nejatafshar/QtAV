TEMPLATE = app
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TRANSLATIONS = res/player_zh_CN.ts
VERSION = $$QTAV_VERSION

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
STATICLINK=1
include($$PWD/../common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)
INCLUDEPATH += $$PWD
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
genRC($$TARGET)

SOURCES += main.cpp \
    MainWindow.cpp \
    Button.cpp \
    ClickableMenu.cpp \
    StatisticsView.cpp \
    Slider.cpp \
    TVView.cpp \
    EventFilter.cpp \
    config/ConfigDialog.cpp \
    config/ConfigPageBase.cpp \
    config/CaptureConfigPage.cpp \
    config/VideoEQConfigPage.cpp \
    config/DecoderConfigPage.cpp \
    config/MiscPage.cpp \
    filters/OSD.cpp \
    filters/OSDFilter.cpp \
    playlist/PlayListModel.cpp \
    playlist/PlayListItem.cpp \
    playlist/PlayListDelegate.cpp \
    playlist/PlayList.cpp \
    config/PropertyEditor.cpp \
    config/AVFormatConfigPage.cpp \
    config/AVFilterConfigPage.cpp \
    filters/AVFilterSubtitle.cpp

HEADERS += \
    MainWindow.h \
    Button.h \
    ClickableMenu.h \
    StatisticsView.h \
    Slider.h \
    TVView.h \
    EventFilter.h \
    config/ConfigDialog.h \
    config/ConfigPageBase.h \
    config/CaptureConfigPage.h \
    config/VideoEQConfigPage.h \
    config/DecoderConfigPage.h \
    config/MiscPage.h \
    filters/OSD.h \
    filters/OSDFilter.h \
    playlist/PlayListModel.h \
    playlist/PlayListItem.h \
    playlist/PlayListDelegate.h \
    playlist/PlayList.h \
    config/PropertyEditor.h \
    config/AVFormatConfigPage.h \
    config/AVFilterConfigPage.h \
    filters/AVFilterSubtitle.h

unix:!android:!mac {
#debian
player_bins = player QMLPlayer
DEB_INSTALL_LIST = $$join(player_bins, \\n.$$[QT_INSTALL_BINS]/, .$$[QT_INSTALL_BINS]/)
DEB_INSTALL_LIST *= \
            usr/share/applications/player.desktop \
            usr/share/applications/QMLPlayer.desktop \
            usr/share/icons/hicolor/64x64/apps/QtAV.svg
deb_install_list.target = qtav-players.install
deb_install_list.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${deb_install_list.target}
QMAKE_EXTRA_TARGETS += deb_install_list
target.depends *= $${deb_install_list.target}

qtav_players_links.target = qtav-players.links
qtav_players_links.commands = echo \"$$[QT_INSTALL_BINS]/player /usr/bin/player\n$$[QT_INSTALL_BINS]/QMLPlayer /usr/bin/QMLPlayer\" >$$PROJECTROOT/debian/$${qtav_players_links.target}
QMAKE_EXTRA_TARGETS *= qtav_players_links
target.depends *= $${qtav_players_links.target}
}

tv.files = res/tv.ini
#BIN_INSTALLS += tv
target.path = $$[QT_INSTALL_BINS]
include($$PROJECTROOT/deploy.pri)

RESOURCES += \
    res/player.qrc \
    theme.qrc
