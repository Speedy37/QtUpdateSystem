QT       += testlib
QT       -= gui

TARGET = tst_packagemanagertests
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_packagemanagertests.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

linux-llvm:QMAKE_CXXFLAGS += -std=c++11
win32-g++:QMAKE_CXXFLAGS += -std=c++11

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

LIBS += -L$$OUT_PWD/../../src/$${subdir} -lQtUpdateSystem
LIBS += -L$$OUT_PWD/../../QtLog/$${subdir} -lQtLog
target.files = $$OUT_PWD/../../src/$${subdir}QtUpdateSystem$${dllext}
target.files += $$OUT_PWD/../../QtLog/$${subdir}QtLog$${dllext}

INCLUDEPATH += $$PWD/../../src
DEPENDPATH += $$PWD/../../src
INCLUDEPATH += $$PWD/../../QtLog
DEPENDPATH += $$PWD/../../QtLog

target.path = $$OUT_PWD/Install
INSTALLS += target

copylzma.commands = $(COPY_FILE) $$shell_path($$PWD/../../lzma.exe) $$shell_path($$OUT_PWD/lzma.exe)
copyxdelta.commands += $(COPY_FILE) $$shell_path($$PWD/../../xdelta3.exe) $$shell_path($$OUT_PWD/xdelta3.exe)
first.depends = $(first) copylzma copyxdelta
export(first.depends)
export(copylzma.commands)
export(copyxdelta.commands)
QMAKE_EXTRA_TARGETS += first copylzma copyxdelta
