QT       += core gui widgets xml
TARGET    = check_form
TEMPLATE  = app
CONFIG   += console c++14
CONFIG   -= app_bundle
DEFINES  += NEXOR_HAS_WIDGETS

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    ../../src/designer/FormCanvas.cpp \
    ../../src/designer/WidgetFactory.cpp

HEADERS += \
    ../../src/designer/FormCanvas.h \
    ../../src/designer/WidgetFactory.h

DESTDIR     = $$PWD/../../../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
