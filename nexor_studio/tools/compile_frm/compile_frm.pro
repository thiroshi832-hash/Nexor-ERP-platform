QT       += core xml sql
QT       -= gui
TARGET    = compile_frm
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
    ../../src/project/Activity.cpp \
    ../../src/project/Sheet.cpp \
    ../../src/project/Process.cpp \
    ../../src/project/BpmnIo.cpp \
    ../../src/project/Project.cpp

HEADERS += \
    ../../src/language/Lexer.h \
    ../../src/language/Parser.h \
    ../../src/language/Value.h \
    ../../src/language/Environment.h \
    ../../src/language/Interpreter.h \
    ../../src/language/NexorRuntime.h \
    ../../src/language/EntityStore.h \
    ../../src/project/Activity.h \
    ../../src/project/Sheet.h \
    ../../src/project/Process.h \
    ../../src/project/BpmnIo.h \
    ../../src/project/Project.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
