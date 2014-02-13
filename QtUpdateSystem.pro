QT       += network

QT       -= gui

TARGET = QtUpdateSystem
TEMPLATE = lib

DEFINES += QTUPDATESYSTEM_LIBRARY LZMA_API_STATIC QTLOG_STATIC

Release:DEFINES += NOLOGDEBUG NOLOGTRACE
linux-llvm:QMAKE_CXXFLAGS += -std=c++11
win32-g++:QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += ./QtLog/

SOURCES += \
    src/operations/addoperation.cpp \
    src/updater/downloadmanager.cpp \
    src/updater/filemanager.cpp \
    src/operations/operation.cpp \
    src/operations/patchoperation.cpp \
    src/operations/removedirectoryoperation.cpp \
    src/operations/removeoperation.cpp \
    src/common/package.cpp \
    src/common/version.cpp \
    src/packagemanager.cpp \
    src/packager.cpp \
    src/common/files.cpp \
    src/common/file.cpp \
    src/updater.cpp \
    QtLog/qtlog.cpp \
    src/common/packages.cpp \
    src/common/jsonutil.cpp \
    src/common/packagemetadata.cpp \
    src/common/versions.cpp \
    src/packager/packagertask.cpp

HEADERS +=\
    src/qtupdatesystem_global.h \
    src/operations/addoperation.h \
    src/updater/downloadmanager.h \
    src/updater/filemanager.h \
    src/operations/operation.h \
    src/operations/patchoperation.h \
    src/operations/removedirectoryoperation.h \
    src/operations/removeoperation.h \
    src/common/package.h \
    src/common/version.h \
    src/packagemanager.h \
    src/packager.h \
    src/common/files.h \
    src/common/file.h \
    src/updater.h \
    QtLog/qtlog.h \
    src/common/packages.h \
    src/common/jsonutil.h \
    src/common/packagemetadata.h \
    src/common/versions.h \
    src/packager/packagertask.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
