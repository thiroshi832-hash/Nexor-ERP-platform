QT       += core xml
QT       -= gui
TARGET    = flux_install
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle

INCLUDEPATH += ../../src ../../../nexor_studio/src

SOURCES += \
    main.cpp \
    ../../src/PackageCache.cpp \
    ../../../nexor_studio/src/build/PackageReader.cpp \
    ../../../nexor_studio/src/project/Activity.cpp \
    ../../../nexor_studio/src/project/Sheet.cpp \
    ../../../nexor_studio/src/project/Process.cpp \
    ../../../nexor_studio/src/project/BpmnIo.cpp \
    ../../../nexor_studio/src/project/Project.cpp

HEADERS += \
    ../../src/PackageCache.h \
    ../../../nexor_studio/src/build/Package.h \
    ../../../nexor_studio/src/build/PackageReader.h \
    ../../../nexor_studio/src/project/Activity.h \
    ../../../nexor_studio/src/project/Sheet.h \
    ../../../nexor_studio/src/project/Process.h \
    ../../../nexor_studio/src/project/BpmnIo.h \
    ../../../nexor_studio/src/project/Project.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
