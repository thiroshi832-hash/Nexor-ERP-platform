QT       += core xml
QT       -= gui
TARGET    = check_forms
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    ../../src/project/Activity.cpp

HEADERS += \
    ../../src/project/Activity.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
