QT       += network
QT       -= gui

TARGET = QtUpdateSystem
TEMPLATE = lib

DEFINES += QTUPDATESYSTEM_LIBRARY LZMA_API_STATIC QTLOG_STATIC

Release:DEFINES += NOLOGDEBUG NOLOGTRACE
linux-llvm:QMAKE_CXXFLAGS += -std=c++11
win32-g++:QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += ../QtLog/

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
    packagemanager.cpp \
    packager.cpp \
    common/files.cpp \
    common/file.cpp \
    updater.cpp \
    common/packages.cpp \
    common/jsonutil.cpp \
    common/packagemetadata.cpp \
    common/versions.cpp \
    packager/packagertask.cpp \
    common/utils.cpp

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
    packagemanager.h \
    packager.h \
    common/files.h \
    common/file.h \
    updater.h \
    common/packages.h \
    common/jsonutil.h \
    common/packagemetadata.h \
    common/versions.h \
    packager/packagertask.h \
    common/utils.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QtLog/release/ -lQtLog
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QtLog/debug/ -lQtLog
else:unix: LIBS += -L$$OUT_PWD/../QtLog/ -lQtLog

INCLUDEPATH += $$PWD/../QtLog
DEPENDPATH += $$PWD/../QtLog
