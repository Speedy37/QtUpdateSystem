QT       += network
QT       -= gui

TARGET = QtUpdateSystem
TEMPLATE = lib

DEFINES += QTUPDATESYSTEM_LIBRARY

Release:DEFINES += NOLOGDEBUG NOLOGTRACE

CONFIG += c++11

SOURCES += \
    operations/addoperation.cpp \
    updater/downloadmanager.cpp \
    updater/filemanager.cpp \
    operations/operation.cpp \
    operations/patchoperation.cpp \
    operations/removedirectoryoperation.cpp \
    operations/removeoperation.cpp \
    common/package.cpp \
    common/version.cpp \
    packager.cpp \
    updater.cpp \
    common/packages.cpp \
    common/jsonutil.cpp \
    common/packagemetadata.cpp \
    common/versions.cpp \
    packager/packagertask.cpp \
    common/utils.cpp \
    repository.cpp \
    updater/copythread.cpp \
    updater/localrepository.cpp \
    errors/warning.cpp

HEADERS +=\
    qtupdatesystem_global.h \
    operations/addoperation.h \
    updater/downloadmanager.h \
    updater/filemanager.h \
    operations/operation.h \
    operations/patchoperation.h \
    operations/removedirectoryoperation.h \
    operations/removeoperation.h \
    common/package.h \
    common/version.h \
    packager.h \
    updater.h \
    common/packages.h \
    common/jsonutil.h \
    common/packagemetadata.h \
    common/versions.h \
    packager/packagertask.h \
    common/utils.h \
    repository.h \
    updater/oneobjectthread.h \
    updater/copythread.h \
    updater/localrepository.h \
    errors/warning.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

win32 {
    exes.files = $$PWD/../xdelta3.exe $$PWD/../lzma.exe
    exes.path = $$[top_builddir]/Install
    INSTALLS += exes
}

target.path = $$[top_builddir]/Install
INSTALLS += target
