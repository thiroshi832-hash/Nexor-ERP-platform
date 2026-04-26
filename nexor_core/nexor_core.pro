QT       += core network
QT       -= gui

TARGET   = NexorCore
TEMPLATE = app
CONFIG  += console c++14
CONFIG  -= app_bundle

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/CoreServer.cpp \
    src/ClientSession.cpp

HEADERS += \
    src/CoreServer.h \
    src/ClientSession.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
