QT       += network
QT       -= gui

TARGET = QtUpdateSystem
TEMPLATE = lib

DEFINES += QTUPDATESYSTEM_LIBRARY LZMA_API_STATIC QTLOG_STATIC

Release:DEFINES += NOLOGDEBUG NOLOGTRACE

CONFIG += c++11

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
    packager.cpp \
    updater.cpp \
    common/packages.cpp \
    common/jsonutil.cpp \
    common/packagemetadata.cpp \
    common/versions.cpp \
    packager/packagertask.cpp \
    common/utils.cpp \
    repository.cpp \
    updater/localrepository.cpp

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
    updater/localrepository.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QtLog/release/ -lQtLog
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QtLog/debug/ -lQtLog
else:unix: LIBS += -L$$OUT_PWD/../QtLog/ -lQtLog

INCLUDEPATH += $$PWD/../QtLog
DEPENDPATH += $$PWD/../QtLog
