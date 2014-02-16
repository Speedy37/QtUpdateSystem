QT       += testlib
QT       -= gui

TARGET = UpdateSystemTests
CONFIG   += console
CONFIG   -= app_bundle

CONFIG += c++11

TEMPLATE = app

DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += tst_packagemanager.cpp \
    tst_packager.cpp \
    main.cpp \
    tst_updater.cpp \
    utils.cpp

win32:CONFIG(release, debug|release) {
    subdir = release/
    dllext = .dll
}
else:win32:CONFIG(debug, debug|release) {
    subdir = debug/
    dllext = .dll
}
else:unix: {
    subdir =
    dllext = .so
}

LIBS += -L$$OUT_PWD/../src/$${subdir} -lQtUpdateSystem
LIBS += -L$$OUT_PWD/../QtLog/$${subdir} -lQtLog
target.files = $$OUT_PWD/../src/$${subdir}QtUpdateSystem$${dllext}
target.files += $$OUT_PWD/../QtLog/$${subdir}QtLog$${dllext}

INCLUDEPATH += $$PWD/../src
DEPENDPATH += $$PWD/../src
INCLUDEPATH += $$PWD/../QtLog
DEPENDPATH += $$PWD/../QtLog

target.path = $$OUT_PWD/Install
INSTALLS += target

HEADERS += \
    tst_packagemanager.h \
    tst_packager.h \
    tst_updater.h \
    utils.h

