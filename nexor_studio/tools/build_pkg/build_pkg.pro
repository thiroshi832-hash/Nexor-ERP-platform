QT       += core xml sql
QT       -= gui
TARGET    = build_pkg
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    ../../src/build/PackageBuilder.cpp \
    ../../src/build/PackageReader.cpp \
    ../../src/project/Project.cpp \
    ../../src/project/Activity.cpp \
    ../../src/project/Sheet.cpp \
    ../../src/project/Process.cpp \
    ../../src/project/BpmnIo.cpp

HEADERS += \
    ../../src/build/Package.h \
    ../../src/build/PackageBuilder.h \
    ../../src/build/PackageReader.h \
    ../../src/project/Project.h \
    ../../src/project/Activity.h \
    ../../src/project/Sheet.h \
    ../../src/project/Process.h \
    ../../src/project/BpmnIo.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
