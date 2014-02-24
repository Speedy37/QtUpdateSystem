QT       += core
QT       -= gui

TARGET = updater
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += c++11

TEMPLATE = app

SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../src/release/ -lQtUpdateSystem
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../src/debug/ -lQtUpdateSystem
else:unix: LIBS += -L$$OUT_PWD/../../src/ -lQtUpdateSystem

INCLUDEPATH += $$PWD/../../src
DEPENDPATH += $$PWD/../../src

target.path = $$[top_builddir]/Install
INSTALLS += target
