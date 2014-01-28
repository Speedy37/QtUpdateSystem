#-------------------------------------------------
#
# Project created by QtCreator 2014-01-20T19:28:10
#
#-------------------------------------------------

QT       -= gui

TARGET = Updater
TEMPLATE = lib

DEFINES += UPDATER_LIBRARY

SOURCES += updater.cpp

HEADERS += updater.h\
        updater_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
