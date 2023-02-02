# QtAV

This repository is a fork of [QtAV](https://github.com/wang-bin/QtAV) which is a multimedia playback library based on Qt and FFmpeg.

## Changes:

- Builds as a unified static lib including qml module
- Ability to record to video files
- Ability to restream to UDP ports
- Added additional statistics for video/audio packets and frames
- Ability to play video streams in real-time without buffering
- Added scripts to get FFmpeg builds
- Fix blocking main event loop for some milliseconds when loading/stopping playback
- Some other modifications and fixes

**QtAV is free software licensed under the term of LGPL v2.1. The player example is licensed under GPL v3.  If you use QtAV or its constituent libraries,
you must adhere to the terms of the license in question.**

## Build
- run `depends.sh` to get FFmpeg builds for current os. You can also get FFmpeg builds for Android or iOS by running `depends_android.sh` or `depends_ios.sh` respectively.
- run:
```
qmake
make
```

## Usage
Link your app with the static build of QtAV:

```
libName = QtAV
android: libName = $${libName}_$${ANDROID_TARGET_ARCH}
LIBS += -L $build_path/QtAV/lib/ -l$${libName}
win32: libName = $${libName}.lib
else:linux: libName = lib$${libName}.a
else:mac:libName = lib$${libName}.a
PRE_TARGETDEPS +=  $build_path/QtAV/lib/$${libName}

INCLUDEPATH += $src_path/QtAV/src
DEPENDPATH += $src_path/QtAV/src
INCLUDEPATH += $src_path/QtAV/qml
DEPENDPATH += $src_path/QtAV/qml
```

To use QtAV in **Qml** register the types:

    qmlRegisterType<QmlAVPlayer>("QtAV", 1, 7, "AVPlayer");
    qmlRegisterType<QuickFBORenderer>("QtAV", 1, 7, "VideoOutput2");
    qmlRegisterType<QtAV::VideoCapture>("QtAV", 1, 7, "VideoCapture");
    qmlRegisterType<MediaMetaData>("QtAV", 1, 7, "MediaMetaData");
