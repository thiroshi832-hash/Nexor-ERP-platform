QT       += core network sql xml
QT       -= gui

TARGET   = NexorCore
TEMPLATE = app
CONFIG  += console c++14
CONFIG  -= app_bundle

INCLUDEPATH += src ../nexor_studio/src

SOURCES += \
    src/main.cpp \
    src/Http.cpp \
    src/PackageRegistry.cpp \
    src/PackageApi.cpp \
    ../nexor_studio/src/build/PackageReader.cpp \
    ../nexor_studio/src/build/PackageDiff.cpp

HEADERS += \
    src/Http.h \
    src/PackageRegistry.h \
    src/PackageApi.h \
    ../nexor_studio/src/build/Package.h \
    ../nexor_studio/src/build/PackageReader.h \
    ../nexor_studio/src/build/PackageDiff.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
