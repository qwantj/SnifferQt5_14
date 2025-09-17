QT       += core gui widgets network

CONFIG   += c++17 windows
CONFIG   -= app_bundle

TARGET   = SnifferQt5
TEMPLATE = app

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    MultiSniffer.cpp \
    PacketModel.cpp

HEADERS += \
    MainWindow.h \
    MultiSniffer.h \
    PacketModel.h

FORMS   += \
    mainwindow.ui

win32-msvc {

    CONFIG -= embed_manifest_exe

    RC_FILE = app_manifest.rc

    QMAKE_LFLAGS += /INCREMENTAL:NO
    QMAKE_LFLAGS_DEBUG += /INCREMENTAL:NO

    LIBS += -lws2_32 -liphlpapi
}

DISTFILES += \
    mainwindow.ui
