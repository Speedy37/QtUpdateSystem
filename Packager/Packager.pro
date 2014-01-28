#-------------------------------------------------
#
# Project created by QtCreator 2014-01-20T20:03:16
#
#-------------------------------------------------

QT       += core gui

TARGET = Packager
TEMPLATE = lib
CONFIG += plugin

DESTDIR = $$[QT_INSTALL_PLUGINS]/generic

SOURCES += packagerplugin.cpp

HEADERS += packagerplugin.h
OTHER_FILES += Packager.json

unix {
    target.path = /usr/lib
    INSTALLS += target
}
