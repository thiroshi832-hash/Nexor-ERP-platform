QT       += core gui widgets network xml sql
DEFINES  += NEXOR_HAS_WIDGETS NEXOR_HAS_PROCESS_ENGINE

TARGET   = NexorFlux
TEMPLATE = app
CONFIG  += c++14

# Flux is a thin shell on top of the Studio runtime.  Rather than duplicate
# the language / project / runtime / form-factory code, we link the same .cpp
# files Studio uses.  Anything we change in Studio gets picked up here on
# the next rebuild.
INCLUDEPATH += src ../nexor_studio/src

SOURCES += \
    src/main.cpp \
    src/FluxWindow.cpp \
    src/CoreClient.cpp \
    src/PackageCache.cpp \
    src/ActivityPicker.cpp \
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
    ../nexor_studio/src/project/Project.cpp \
    \
    ../nexor_studio/src/runtime/FormRunner.cpp \
    ../nexor_studio/src/runtime/WidgetValue.cpp \
    ../nexor_studio/src/runtime/ProcessEngine.cpp \
    \
    ../nexor_studio/src/designer/WidgetFactory.cpp

HEADERS += \
    src/FluxWindow.h \
    src/CoreClient.h \
    src/PackageCache.h \
    src/ActivityPicker.h \
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
    ../nexor_studio/src/project/Project.h \
    \
    ../nexor_studio/src/runtime/FormRunner.h \
    ../nexor_studio/src/runtime/WidgetValue.h \
    ../nexor_studio/src/runtime/ProcessEngine.h \
    \
    ../nexor_studio/src/designer/WidgetFactory.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
