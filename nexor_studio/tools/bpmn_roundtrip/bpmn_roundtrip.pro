QT       += core xml
QT       -= gui
TARGET    = bpmn_roundtrip
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    ../../src/project/Process.cpp \
    ../../src/project/BpmnIo.cpp

HEADERS += \
    ../../src/project/Process.h \
    ../../src/project/BpmnIo.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
