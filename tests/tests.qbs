import qbs 1.0

CppApplication {
    Depends { name: "Qt"; submodules: ["core", "test"] }
    Depends { name: "qtupdatesystem" }
    name : "tests"
    type: "application"
    consoleApplication: true
    cpp.defines: [
        "SRCDIR=\""+path+"/\""
    ]
    files : [
        "main.cpp",
        "testutils.cpp",
        "testutils.h",
        "tst_packager.cpp",
        "tst_packager.h",
        "tst_repository.cpp",
        "tst_repository.h",
        "tst_updatechain.cpp",
        "tst_updatechain.h",
        "tst_updater.cpp",
        "tst_updater.h"
    ]
    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.binInstallDir || ""
    }
}
