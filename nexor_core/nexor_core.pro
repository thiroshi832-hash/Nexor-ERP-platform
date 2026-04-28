QT       += core network sql xml
QT       -= gui

TARGET   = NexorCore
TEMPLATE = app
CONFIG  += console c++14
CONFIG  -= app_bundle

INCLUDEPATH += src ../nexor_studio/src

# Core needs to actually run [ServerOnly] activities, so it links the same
# language + runtime sources Studio uses.  Same INCLUDEPATH trick we use
# for PackageReader.
SOURCES += \
    src/main.cpp \
    src/Http.cpp \
    src/PackageRegistry.cpp \
    src/PackageApi.cpp \
    src/RpcApi.cpp \
    src/ValueJson.cpp \
    \
    ../nexor_studio/src/build/PackageReader.cpp \
    ../nexor_studio/src/build/PackageDiff.cpp \
    \
    ../nexor_studio/src/language/Lexer.cpp \
    ../nexor_studio/src/language/Parser.cpp \
    ../nexor_studio/src/language/Value.cpp \
    ../nexor_studio/src/language/Environment.cpp \
    ../nexor_studio/src/language/Interpreter.cpp \
    ../nexor_studio/src/language/NexorRuntime.cpp \
    ../nexor_studio/src/language/EntityStore.cpp \
    \
    ../nexor_studio/src/project/Activity.cpp \
    ../nexor_studio/src/project/Sheet.cpp \
    ../nexor_studio/src/project/Process.cpp \
    ../nexor_studio/src/project/BpmnIo.cpp \
    ../nexor_studio/src/project/Project.cpp

HEADERS += \
    src/Http.h \
    src/PackageRegistry.h \
    src/PackageApi.h \
    src/RpcApi.h \
    src/ValueJson.h \
    \
    ../nexor_studio/src/build/Package.h \
    ../nexor_studio/src/build/PackageReader.h \
    ../nexor_studio/src/build/PackageDiff.h \
    \
    ../nexor_studio/src/language/Token.h \
    ../nexor_studio/src/language/Ast.h \
    ../nexor_studio/src/language/Lexer.h \
    ../nexor_studio/src/language/Parser.h \
    ../nexor_studio/src/language/Value.h \
    ../nexor_studio/src/language/Environment.h \
    ../nexor_studio/src/language/Interpreter.h \
    ../nexor_studio/src/language/NexorRuntime.h \
    ../nexor_studio/src/language/EntityStore.h \
    \
    ../nexor_studio/src/project/Activity.h \
    ../nexor_studio/src/project/Sheet.h \
    ../nexor_studio/src/project/Process.h \
    ../nexor_studio/src/project/BpmnIo.h \
    ../nexor_studio/src/project/Project.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
