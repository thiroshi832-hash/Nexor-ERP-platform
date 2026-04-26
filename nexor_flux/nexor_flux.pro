QT       += core gui widgets network

TARGET   = NexorFlux
TEMPLATE = app
CONFIG  += c++14

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/FluxWindow.cpp \
    src/CoreClient.cpp

HEADERS += \
    src/FluxWindow.h \
    src/CoreClient.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
