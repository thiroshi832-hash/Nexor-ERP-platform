QT       += core gui widgets xml sql network

TARGET   = NexorStudio
TEMPLATE = app
CONFIG  += c++14

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/ide/MainWindow.cpp \
    src/ide/FancyTabBar.cpp \
    src/ide/CentralStack.cpp \
    src/ide/OutputPane.cpp \
    src/welcome/WelcomePage.cpp \
    src/editor/CodeEditor.cpp \
    src/editor/EditorView.cpp \
    src/editor/NexorHighlighter.cpp \
    src/designer/WidgetFactory.cpp \
    src/designer/FormCanvas.cpp \
    src/designer/PropertyPanel.cpp \
    src/designer/WidgetPalette.cpp \
    src/designer/DesignerView.cpp \
    src/runtime/FormRunner.cpp \
    src/runtime/WidgetValue.cpp \
    src/runtime/ProcessEngine.cpp \
    src/build/PackageBuilder.cpp \
    src/build/PackageReader.cpp \
    src/build/PackageDiff.cpp \
    src/language/Lexer.cpp \
    src/language/Parser.cpp \
    src/language/Value.cpp \
    src/language/Environment.cpp \
    src/language/Interpreter.cpp \
    src/language/NexorRuntime.cpp \
    src/project/Project.cpp \
    src/project/Activity.cpp \
    src/project/Sheet.cpp \
    src/project/Process.cpp \
    src/project/ProjectTree.cpp \
    src/sheet/SheetEditor.cpp \
    src/process/ProcessEditor.cpp \
    src/dialogs/NewProjectDialog.cpp \
    src/dialogs/NewActivityDialog.cpp \
    src/dialogs/NewSheetDialog.cpp \
    src/dialogs/NewProcessDialog.cpp \
    src/dialogs/TabOrderDialog.cpp \
    src/language/EntityStore.cpp

HEADERS += \
    src/ide/MainWindow.h \
    src/ide/FancyTabBar.h \
    src/ide/CentralStack.h \
    src/ide/OutputPane.h \
    src/welcome/WelcomePage.h \
    src/editor/CodeEditor.h \
    src/editor/EditorView.h \
    src/editor/NexorHighlighter.h \
    src/designer/WidgetFactory.h \
    src/designer/FormCanvas.h \
    src/designer/PropertyPanel.h \
    src/designer/WidgetPalette.h \
    src/designer/DesignerView.h \
    src/runtime/FormRunner.h \
    src/runtime/WidgetValue.h \
    src/runtime/ProcessEngine.h \
    src/build/Package.h \
    src/build/PackageBuilder.h \
    src/build/PackageReader.h \
    src/build/PackageDiff.h \
    src/language/Token.h \
    src/language/Lexer.h \
    src/language/Ast.h \
    src/language/Parser.h \
    src/language/Value.h \
    src/language/Environment.h \
    src/language/Interpreter.h \
    src/language/NexorRuntime.h \
    src/project/Project.h \
    src/project/Activity.h \
    src/project/Sheet.h \
    src/project/Process.h \
    src/project/ProjectTree.h \
    src/sheet/SheetEditor.h \
    src/process/ProcessEditor.h \
    src/dialogs/NewProjectDialog.h \
    src/dialogs/NewActivityDialog.h \
    src/dialogs/NewSheetDialog.h \
    src/dialogs/NewProcessDialog.h \
    src/dialogs/TabOrderDialog.h \
    src/language/EntityStore.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
