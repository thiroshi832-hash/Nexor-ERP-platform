QT       += core gui widgets xml

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
    src/project/Project.cpp \
    src/project/Activity.cpp \
    src/project/ProjectTree.cpp \
    src/dialogs/NewProjectDialog.cpp \
    src/dialogs/NewActivityDialog.cpp

HEADERS += \
    src/ide/MainWindow.h \
    src/ide/FancyTabBar.h \
    src/ide/CentralStack.h \
    src/ide/OutputPane.h \
    src/welcome/WelcomePage.h \
    src/project/Project.h \
    src/project/Activity.h \
    src/project/ProjectTree.h \
    src/dialogs/NewProjectDialog.h \
    src/dialogs/NewActivityDialog.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
