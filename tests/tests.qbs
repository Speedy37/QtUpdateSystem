import qbs 1.0

CppApplication {
    Depends { name: "Qt"; submodules: ["core", "test"] }
    Depends { name: "qtupdatesystem" }
    name : "tests"
    consoleApplication: true
    cpp.defines: [
        "SRCDIR=\""+path+"/\""
    ]

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: ["/W3", "/w34100", "/w34189"]
    }
    Properties {
        condition: qbs.toolchain.contains("llvm") || qbs.toolchain.contains("gcc")
        cpp.cxxFlags: ["-std=c++11"]
    }

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
