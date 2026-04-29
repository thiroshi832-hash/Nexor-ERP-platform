QT       += core xml sql
QT       -= gui
TARGET    = lang_smoke
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    ../../src/language/Lexer.cpp \
    ../../src/language/Parser.cpp \
    ../../src/language/Value.cpp \
    ../../src/language/Environment.cpp \
    ../../src/language/Interpreter.cpp \
    ../../src/language/NexorRuntime.cpp \
    ../../src/language/EntityStore.cpp \
    ../../src/project/Project.cpp \
    ../../src/project/Activity.cpp \
    ../../src/project/Sheet.cpp \
    ../../src/project/Process.cpp \
    ../../src/project/BpmnIo.cpp

HEADERS += \
    ../../src/language/NexorRuntime.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
