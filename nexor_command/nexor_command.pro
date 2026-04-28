QT       += core gui widgets network

TARGET   = NexorCommand
TEMPLATE = app
CONFIG  += c++14

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/CoreClient.cpp \
    src/SettingsDialog.cpp

HEADERS += \
    src/MainWindow.h \
    src/CoreClient.h \
    src/SettingsDialog.h

DESTDIR     = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
