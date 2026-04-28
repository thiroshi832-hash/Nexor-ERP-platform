QT       += core gui widgets network

TARGET   = NexorCommand
TEMPLATE = app
CONFIG  += c++14

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/CoreClient.cpp \
    src/SettingsDialog.cpp \
    src/DiffDialog.cpp \
    src/AuditDialog.cpp

HEADERS += \
    src/MainWindow.h \
    src/CoreClient.h \
    src/SettingsDialog.h \
    src/DiffDialog.h \
    src/AuditDialog.h

RC_FILE     = nexor_command.rc
RESOURCES  += nexor_command.qrc

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
