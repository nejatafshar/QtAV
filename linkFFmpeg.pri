win32:{
  contains(QT_ARCH, i386): {
    INCLUDEPATH += $$PWD/ffmpegBin/win/32/include
    LIBS += -L$$PWD/ffmpegBin/win/32/lib
  }else{
    INCLUDEPATH += $$PWD/ffmpegBin/win/64/include
    LIBS += -L$$PWD/ffmpegBin/win/64/lib
  }
}
else:linux:!android:{
  contains(QT_ARCH, i386): {
    INCLUDEPATH += $$PWD/ffmpegBin/linux/32/include
    LIBS += -L$$PWD/ffmpegBin/linux/32/lib
  }else{
    INCLUDEPATH += $$PWD/ffmpegBin/linux/64/include
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lavdevice
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lavfilter
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lavformat
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lavcodec
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lavutil
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lswresample
    LIBS += -L$$PWD/ffmpegBin/linux/64/lib/ -lswscale
    LIBS += -lvdpau -ldl
  }
}
else:android:{
    INCLUDEPATH += $$PWD/ffmpegBin/Android/clang/include

    equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lavfilter
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lavformat
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lavcodec
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lavutil
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lswresample
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/ -lswscale
    }
    equals(ANDROID_TARGET_ARCH, arm64-v8a) {
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lavfilter
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lavformat
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lavcodec
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lavutil
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lswresample
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/arm64-v8a/ -lswscale
    }
    equals(ANDROID_TARGET_ARCH, x86) {
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lavfilter
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lavformat
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lavcodec
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lavutil
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lswresample
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86/ -lswscale
    }
    equals(ANDROID_TARGET_ARCH, x86_64) {
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lavfilter
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lavformat
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lavcodec
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lavutil
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lswresample
        LIBS += -L$$PWD/ffmpegBin/Android/clang/lib/x86_64/ -lswscale
    }

}
else:macx:{
    INCLUDEPATH += $$PWD/ffmpegBin/linux/64/include
    LIBS += -L$$PWD/ffmpegBin/mac/lib
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lavdevice
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lavfilter
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lavformat
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lavcodec
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lavutil
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lswresample
    LIBS += -L$$PWD/ffmpegBin/mac/lib/ -lswscale
}
else:mac:{ #ios
    INCLUDEPATH += $$PWD/ffmpegBin/linux/64/include
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lavfilter
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lavformat
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lavcodec
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lavutil
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lswresample
    LIBS += -L$$PWD/ffmpegBin/ios/lib/ -lswscale
}
