#-------------------------------------------------
#
# Project created by QtCreator 2021-09-06T10:56:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpegCameraDemo
TEMPLATE = app

contains(QT_ARCH, i386) {
    message("32-bit")
    CPU_ARCH = x86
    SYS_BITS = 32
} else {
    message("64-bit")
    CPU_ARCH = x64
    SYS_BITS = 64
}

DESTDIR = bin

win32 {
INCLUDEPATH += $$PWD/../ffmpeg/include
LIBS += -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH}  -lavcodec\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lavdevice\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lavfilter\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lavformat\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lavutil\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lswresample\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lswscale\
      -L$$PWD/../ffmpeg/lib/msvc/$${CPU_ARCH} -lpostproc
}

unix{
INCLUDEPATH+=/home/chw/HSCompany/videoCall/ffmpeg/include
LIBS+=-L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lx264\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lva\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lSDL2\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lva-drm\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lva-x11\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lvdpau\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lavcodec\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lavdevice\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lavfilter\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lavformat\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lavutil\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lswresample\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lswscale\
      -L/home/chw/HSCompany/videoCall/ffmpeg/lib/ -lpostproc
LIBS += -lz
LIBS += -lXext
LIBS += -lX11
LIBS += -lxcb
}


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    cgusbcamera.cpp \
    cgusbrecord.cpp

HEADERS += \
        mainwindow.h \
    cgusbcamera.h \
    cgusbrecord.h \
    cgusbcameracustomevent.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
